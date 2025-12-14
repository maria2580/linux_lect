#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<sys/mman.h>
#include<sys/wait.h>
#include<time.h>

#define NUM_PROCESSES 10
#define TIME_QUANTUM 1
 // 라운드 로빈을 위한 타임 퀀텀
#define SIGUSR3 (SIGRTMIN + 0) // 사용자 정의 시그널 3 추가

typedef struct {
    int pids[NUM_PROCESSES];
    int front;
    int rear;
    int count;
} ProcessQueue;

typedef struct {
    pid_t pid;
    int burst_time;
    int remaining_time;
    int time_quantum_left;
    int io_wait_time;
    int total_wait_time; // 대기 시간 누적
    int state; // 0: ready, 1: running, 2: waiting, 3: terminated
} PCB;

// 공유 메모리 구조체
typedef struct {
    int current_process_idx;
    PCB process_table[NUM_PROCESSES];
    ProcessQueue ready_queue;
    ProcessQueue sleep_queue;
} SharedSchedulerState;

SharedSchedulerState *state;
int my_idx = -1; // 자식 프로세스가 자신의 인덱스를 기억하기 위한 전역 변수

// 큐 함수
void init_queue(ProcessQueue *q) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
}

void enqueue(ProcessQueue *q, int idx) {
    if (q->count >= NUM_PROCESSES) return;
    q->pids[q->rear] = idx;
    q->rear = (q->rear + 1) % NUM_PROCESSES;
    q->count++;
}

int dequeue(ProcessQueue *q) {
    if (q->count == 0) return -1;
    int idx = q->pids[q->front];
    q->front = (q->front + 1) % NUM_PROCESSES;
    q->count--;
    return idx;
}

void remove_from_queue(ProcessQueue *q, int idx_to_remove) {
    if (q->count == 0) return;
    
    int found_pos = -1;
    int current = q->front;
    for (int i = 0; i < q->count; i++) {
        if (q->pids[current] == idx_to_remove) {
            found_pos = i;
            break;
        }
        current = (current + 1) % NUM_PROCESSES;
    }

    if (found_pos != -1) {
        int pos = (q->front + found_pos) % NUM_PROCESSES;
        for (int i = found_pos; i < q->count - 1; i++) {
            int next = (pos + 1) % NUM_PROCESSES;
            q->pids[pos] = q->pids[next];
            pos = next;
        }
        q->rear = (q->rear - 1 + NUM_PROCESSES) % NUM_PROCESSES;
        q->count--;
    }
}

// 함수 선언




// 자식 프로세스가 자신의 CPU burst가 끝났을 때 호출됨
void child_action_handler(int signo) {
    if (my_idx == -1) return;

    PCB *my_pcb = &state->process_table[my_idx];
    my_pcb->remaining_time--;

    printf("프로세스 %d 실행 중. 남은 시간: %d\n", my_idx, my_pcb->remaining_time);

    if (my_pcb->remaining_time <= 0) {
        if (rand() % 2) { // 50% 확률로 I/O 요청
            kill(getppid(), SIGUSR1);
        } else {
            exit(0);
        }
    } else {
        // 작업이 아직 남았음 -> 부모에게 "작업 완료" 알림 (SIGUSR3)
        kill(getppid(), SIGUSR3);
    }
}
void make_process(int index) {
    pid_t chldPid = fork();
    if (chldPid == 0) { // 자식 프로세스
        srand(42+index); // 시드 설정
        my_idx = index; // 자신의 인덱스 저장
        signal(SIGUSR2, child_action_handler);
        while(1) {
            pause(); // 부모의 시그널을 기다림
        }
    } else { // 부모 프로세스
        PCB *new_pcb = &state->process_table[index];
        new_pcb->pid = chldPid;
        new_pcb->burst_time = rand() % 10 + 1;
        new_pcb->remaining_time = new_pcb->burst_time;
        new_pcb->time_quantum_left = TIME_QUANTUM;
        new_pcb->io_wait_time = 0;
        new_pcb->total_wait_time = 0; // 초기화
        new_pcb->state = 0; // ready
        enqueue(&state->ready_queue, index); // 레디 큐에 추가
        printf("프로세스 %d 생성 (PID: %d, Burst: %d)\n", index, chldPid, new_pcb->burst_time);
    }
}
// 부모의 SIGUSR3 핸들러 (자식의 일반 작업 완료 알림)
void handle_sigusr3(int signo) {
    // 아무것도 안 함. pause()를 깨우기 위한 용도.
}

// 부모의 I/O 요청 핸들러
void handle_io_request(int signo, siginfo_t *info, void *context) {
    pid_t child_pid = info->si_pid;
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (state->process_table[i].pid == child_pid) {
            PCB* pcb = &state->process_table[i];
            pcb->state = 2; // waiting
            pcb->io_wait_time = rand() % 5 + 1; // 1~5초의 I/O 대기시간 할당
            enqueue(&state->sleep_queue, i); // 슬립 큐에 추가
            printf("스케줄러: 프로세스 %d (PID: %d) I/O 요청. %d초 대기 (Sleep Queue 추가)\n", i, child_pid, pcb->io_wait_time);
            
            if(state->current_process_idx == i){
                state->current_process_idx = -1; // 현재 실행 중인 프로세스가 I/O 요청을 했으므로, CPU는 비어있음
            }
            break;
        }
    }
}

// 부모의 자식 종료 핸들러
void handle_child_exit(int signo) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (state->process_table[i].pid == pid) {
                state->process_table[i].state = 3; // terminated
                printf("스케줄러: 프로세스 %d (PID: %d) 종료됨.\n", i, pid);
                if(state->current_process_idx == i){
                   state->current_process_idx = -1;
                }
                break;
            }
        }
    }
}

// 메인 스케줄러 로직 (타이머에 의해 1초마다 실행)
void schedule_tick(int signo) {
    printf("\n--- Tick ---\n");

    // 0. Ready Queue에 있는 프로세스들의 대기 시간 증가
    int ready_count = state->ready_queue.count;
    int r_idx_ptr = state->ready_queue.front;
    for(int i=0; i<ready_count; i++){
        int p_idx = state->ready_queue.pids[r_idx_ptr];
        state->process_table[p_idx].total_wait_time++;
        r_idx_ptr = (r_idx_ptr + 1) % NUM_PROCESSES;
    }

    printf("현재 실행 중: %d\n", state->current_process_idx);
    printf("현재 레디큐: ");
    for (int i = 0; i < state->ready_queue.count; i++) {
        int idx = (state->ready_queue.front + i) % NUM_PROCESSES;
        printf("%d ", state->ready_queue.pids[idx]);
    }
    printf("\n");
    printf("현재 슬립큐: ");
    for (int i = 0; i < state->sleep_queue.count; i++)
    {
        int idx = (state->sleep_queue.front + i) % NUM_PROCESSES;
        printf("%d ", state->sleep_queue.pids[idx]);
    }
    printf("\n");

    // 1. Sleep Queue 업데이트
    int sleeping_pids[NUM_PROCESSES];
    int sleep_count = state->sleep_queue.count;
    int idx_ptr = state->sleep_queue.front;
    
    // 현재 슬립 큐의 내용을 복사 (순회하며 수정하기 위해)
    for(int i=0; i<sleep_count; i++){
        sleeping_pids[i] = state->sleep_queue.pids[idx_ptr];
        idx_ptr = (idx_ptr + 1) % NUM_PROCESSES;
    }

    for (int i = 0; i < sleep_count; i++) {
        int p_idx = sleeping_pids[i];
        PCB *pcb = &state->process_table[p_idx];
        
        pcb->io_wait_time--;
        printf("프로세스 %d I/O 대기 중 (%d초 남음)\n", p_idx, pcb->io_wait_time);
        
        if (pcb->io_wait_time <= 0) {
            remove_from_queue(&state->sleep_queue, p_idx); // 슬립 큐에서 제거
            pcb->state = 0; // ready 상태로 변경
            pcb->burst_time = rand() % 10 + 1; 
            pcb->remaining_time = pcb->burst_time;
            enqueue(&state->ready_queue, p_idx); // 레디 큐에 추가
            printf("프로세스 %d I/O 완료. Ready Queue로 이동.\n", p_idx);
        }
    }
    // 2. 라운드 로빈: Ready Queue에서 다음 프로세스 선택
    if (state->current_process_idx == -1) {
        // 퀀텀이 남아있는 프로세스를 찾을 때까지 dequeue -> enqueue 반복
        int initial_count = state->ready_queue.count;
        int found_idx = -1;

        for (int i = 0; i < initial_count; i++) {
            int idx = dequeue(&state->ready_queue);
            if (idx == -1) break;

            if (state->process_table[idx].time_quantum_left > 0) {
                found_idx = idx;
                break;
            } else {
                // 퀀텀이 없으면 다시 뒤로 보냄 (이번 턴에는 실행 불가)
                enqueue(&state->ready_queue, idx);
            }
        }

        if (found_idx != -1) {
            state->process_table[found_idx].state = 1; // running
            // [수정] 여기서 리필하지 않음! (Epoch 방식)
            // state->process_table[found_idx].time_quantum_left = TIME_QUANTUM; 
            state->current_process_idx = found_idx;
            printf("스케줄러: Ready Queue에서 프로세스 %d (PID: %d) 선택하여 실행 (남은 퀀텀: %d)\n", 
                   found_idx, state->process_table[found_idx].pid, state->process_table[found_idx].time_quantum_left);
        } else {
            printf("스케줄러: 실행 가능한 프로세스가 없음 (모두 퀀텀 소진 or 큐 비어있음)\n");
        }
    }
    
    // 3. 현재 실행 중인 프로세스 업데이트
    int current_idx = state->current_process_idx;
    if (current_idx != -1 && state->process_table[current_idx].state == 1) {
        PCB *current_proc = &state->process_table[current_idx];
        
        // 자식에게 실행 명령 전송
        kill(current_proc->pid, SIGUSR2);
        
        // 자식의 응답(SIGUSR1(I/O요청), SIGUSR3(연산 완료), SIGCHLD(종료됨))을 기다림
        pause();

        // 자식이 종료되었거나 I/O를 요청해서 상태가 변했는지 확인
        if (state->process_table[current_idx].state != 1) {
            // 이미 핸들러(handle_io_request, handle_child_exit)에 의해 처리됨
            // current_process_idx도 -1로 변경되었을 것임
        } else {
            // 자식이 여전히 Running 상태라면 (SIGUSR3 수신)
            current_proc->time_quantum_left--;
            printf("프로세스 %d 실행 중. 남은 퀀텀: %d\n", current_idx, current_proc->time_quantum_left);

            if (current_proc->time_quantum_left <= 0) {
                // 타임 퀀텀 만료, context switch 필요
                current_proc->state = 0; // Ready 상태로
                enqueue(&state->ready_queue, current_idx); // 레디 큐 맨 뒤로 이동
                state->current_process_idx = -1;
                printf("프로세스 %d 타임 퀀텀 만료. Ready Queue로 이동.\n", current_idx);
            }
        }
    }

   
    // 4. 모든 프로세스가 종료되었는지 확인
    int all_done = 1;
    for(int i=0; i < NUM_PROCESSES; i++){
        if(state->process_table[i].state != 3){
            all_done = 0;
            break;
        }
    }
    if(all_done){
        printf("\n모든 프로세스가 종료되었습니다.\n");
        
        // 평균 대기 시간 계산 및 출력
        int total_wait_sum = 0;
        for(int i=0; i<NUM_PROCESSES; i++){
            printf("Process %d Total Wait Time: %d\n", i, state->process_table[i].total_wait_time);
            total_wait_sum += state->process_table[i].total_wait_time;
        }
        printf("평균 대기 시간: %.2f\n", (float)total_wait_sum / NUM_PROCESSES);

        exit(0);
    }

    // 5. 모든 프로세스의 타임 퀀텀이 0인지 확인하고 일괄 초기화 (Epoch)
    int all_quantum_zero = 1;
    for (int i = 0; i < NUM_PROCESSES; i++) {
        // 종료되지 않은 프로세스 중 퀀텀이 남은 게 하나라도 있으면 초기화 안 함
        if (state->process_table[i].state != 3 && state->process_table[i].time_quantum_left > 0) {
            all_quantum_zero = 0;
            break;
        }
    }

    if (all_quantum_zero) {
        printf("\n[Epoch] 모든 프로세스 퀀텀 소진 -> 전체 퀀텀 리필!\n");
        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (state->process_table[i].state != 3) {
                state->process_table[i].time_quantum_left = TIME_QUANTUM;
            }
        }
    }

    alarm(1); // 다음 tick을 위한 알람 설정
}

int main() {
    // 공유 메모리 설정
    state = mmap(NULL, sizeof(SharedSchedulerState), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (state == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    state->current_process_idx = -1;
    init_queue(&state->ready_queue);
    init_queue(&state->sleep_queue);
    srand(42);

    // 시그널 핸들러 설정
    struct sigaction sa_io, sa_chld;
    
    // 자식의 I/O 요청을 처리할 핸들러 (SIGUSR1)
    sa_io.sa_sigaction = handle_io_request;
    sa_io.sa_flags = SA_SIGINFO;
    sigemptyset(&sa_io.sa_mask);
    sigaction(SIGUSR1, &sa_io, NULL);

    // 종료된 자식을 처리할 핸들러 (SIGCHLD)
    sa_chld.sa_handler = handle_child_exit;
    sa_chld.sa_flags = SA_RESTART;
    sigemptyset(&sa_chld.sa_mask);
    sigaction(SIGCHLD, &sa_chld, NULL);
    
    // 스케줄러 tick을 위한 핸들러 (SIGALRM)
    signal(SIGALRM, schedule_tick);

    // 자식의 일반 작업 완료 알림 핸들러 (SIGUSR3)
    signal(SIGUSR3, handle_sigusr3);

    // 프로세스 생성
    for (int i = 0; i < NUM_PROCESSES; i++) {
        make_process(i);
    }

    printf("스케줄러를 시작합니다...\n");
    alarm(1); // 스케줄러 시작

    while (1) {
        pause(); // 시그널 대기
    }
    return 0;
}