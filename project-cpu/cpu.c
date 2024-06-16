#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_PROCESS 5
#define MAX_QUEUE 5
#define MAX_SECOND 150
#define TQ 3 // time quantum

typedef struct {
    int data[MAX_QUEUE];
    int front;
    int rear;
    int size;
}QUEUE;

typedef struct  {
    int pid; // Process ID
    int cpu_burst; // CPU burst time
    int io_burst; // I/O burst time
    int arrival_time; // Arrival time
    int priority; // Priority
}PROCESS;

PROCESS processes[MAX_PROCESS];

QUEUE readyQ;
QUEUE readyQ_FCFS;
QUEUE readyQ_SJF;
QUEUE readyQ_prt;
QUEUE readyQ_RR;
QUEUE readyQ_prmtSJF;
QUEUE waitingQ; // I/O 상태일 때 이쪽에 들어감

void Make_queue(QUEUE *q) {
    q->front = 0;
    q->rear = 0;
    q->size = 0;
}

int Is_full(QUEUE *q) {
    if (q->size == MAX_QUEUE) return 1;
    return 0;
}

int Is_empty(QUEUE *q) {
    if (q->size == 0) return 1;
    return 0;
}

int Dequeue(QUEUE *q) {
    if (Is_empty(q)) {
        printf("ERROR - Queue is empty!\n");
        return -1;
    }
    
    int val = q->data[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->size--;
    return val;
}

int Enqueue(QUEUE *q, int a) {
    if (Is_full(q)) {
        printf("ERROR - Queue is full!\n");
        return -1;
    }
    q->data[q->rear] = a;
    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->size++;
    return 0;
} 

void Sort_queue(QUEUE *q) { // Sort Q by arrival_time
    for (int i = 0; i < q->size - 1; i++) {
        for (int j = 0; j < q->size - i - 1; j++) {
            if (processes[q->data[j]-1].arrival_time > processes[q->data[j+1]-1].arrival_time) {
                int temp = q->data[j];
                q->data[j] = q->data[j+1];
                q->data[j+1] = temp;
            }
        }
    }
}

void Create_Process() { // Assign data to processes
    srand(time(NULL));
    
    for (int i = 0; i < MAX_PROCESS; i++) {
        processes[i].pid = i+1;
        processes[i].cpu_burst = rand() % 10 + 1; // 1~10
        processes[i].io_burst = rand() % 5; // 0~4
        processes[i].arrival_time = rand() % 10; // 0~9
        processes[i].priority = rand() % 5 + 1; // 1~5
    }
}

void Config() { // ready queue & waiting queue
    Make_queue(&readyQ);
    Make_queue(&readyQ_FCFS);
    Make_queue(&readyQ_SJF);
    Make_queue(&readyQ_prt);
    Make_queue(&readyQ_RR);
    Make_queue(&readyQ_prmtSJF);
    Make_queue(&waitingQ);

    for (int i = 0; i < MAX_PROCESS; i++) {
        Enqueue(&readyQ, processes[i].pid);
        Enqueue(&readyQ_FCFS, processes[i].pid);
        Enqueue(&readyQ_SJF, processes[i].pid);
        Enqueue(&readyQ_prt, processes[i].pid);
        Enqueue(&readyQ_RR, processes[i].pid);
        Enqueue(&readyQ_prmtSJF, processes[i].pid);
    }
    Sort_queue(&readyQ); 
    Sort_queue(&readyQ_FCFS); 
    Sort_queue(&readyQ_SJF); 
    Sort_queue(&readyQ_prt); 
    Sort_queue(&readyQ_RR);
    Sort_queue(&readyQ_prmtSJF); 
}

typedef struct {
    int* second;
    int end_process;
} SCHEDULED_RESULT;

SCHEDULED_RESULT FCFS() {
    //printf("FCFS is started!\n");
    //readyQ의 첫번째 원소를 프로세스로 선택
    PROCESS current_process = processes[readyQ_FCFS.data[0] - 1];
    
    int *second = (int*)malloc(MAX_SECOND * sizeof(int)); //second 배열의 크기는 전체 프로세스 처리 시간
    int sec_index = 0;
    int process_index = 0;
    int current_cpu_burst = current_process.cpu_burst; //현재 프로세스의 cpuburst를 따로 분리해 현재 burst로 선언
    int end_second; // 마지막 프로세스가 끝나는 시간
    
    //0초부터 첫 번째 프로세스의 도착 시간까지의 기록
    if (current_process.arrival_time > 0) { 
        for (int i = 0; i < current_process.arrival_time; i++) {
            second[i] = 0; 
        }
        sec_index = current_process.arrival_time;
    }

    //printf("1) sec_index = %d\n", sec_index);
    //printf("2) current_cpu_burst = %d\n", current_cpu_burst);
    //printf("3) current process = %d\n", current_process.pid);
    while(!Is_empty(&readyQ_FCFS)) { //ready queue가 빌 때까지 반복
        //printf("[while]\n");
        //현재 프로세스의 도착 시간이 second보다 큰 경우 : 실행할 프로세스가 없음 => 해당 second부터 도착시간까지 0 기록
        if (current_process.arrival_time > sec_index) {
            for (; sec_index < current_process.arrival_time; sec_index++) {
                second[sec_index] = 0; // second 배열 0 : 처리할 프로세스 없음(pid는 1~9)
            }
            continue;
        }
        //현재 burst의 값이 0 : 실행 완료 => 디큐하고 다음 프로세스 선택, 현재 burst 재설정
        if (current_cpu_burst == 0) { // Process 실행 완료
            Dequeue(&readyQ_FCFS);
            if(!Is_empty(&readyQ_FCFS)){
                current_process = processes[readyQ_FCFS.data[++process_index]-1];
                current_cpu_burst = current_process.cpu_burst;
                //printf("Next process changed.\n");
                //printf("Next process : %d | CPU burst : %d\n", current_process.pid, current_cpu_burst);
            }
            else {
                end_second = sec_index;
                //printf("Processing Ended.\n");
                break;
            }
        }
        //현재 burst의 값이 0 아니면 실행중 => 현재 burst--
        else { // Process 실행 중
            second[sec_index] = current_process.pid;
            current_cpu_burst--;
            sec_index++;
            //printf("Processing...%d | current burst : %d | sec = %d\n", current_process.pid, current_cpu_burst, sec_index);
        }
    }

    SCHEDULED_RESULT result = {second, end_second};
    return result;
}

SCHEDULED_RESULT SJF() {
    //printf("SJF is started!\n");

    //second 배열 선언 
    int *second = (int*)malloc(MAX_SECOND * sizeof(int)); //second 배열의 크기는 전체 프로세스 처리 시간
    int sec_index = 0;
    int end_second;

    while(!Is_empty(&readyQ_SJF)) {
        //printf("[while]\n");
        int shortest_index = -1;
        int shortest_burst = 100; // inf로 초기화하는 것과 같음

        for (int i = 0; i < readyQ_SJF.size; i++) {
            PROCESS p = processes[readyQ_SJF.data[i] - 1]; 
            if ((p.cpu_burst < shortest_burst) && (p.arrival_time <= sec_index)) {
                shortest_burst = p.cpu_burst;
                shortest_index = i;
            }
        }
        //printf("shortest process = %d, burst = %d\n", readyQ_SJF.data[shortest_index], shortest_burst);
        // 가장 짧은 프로세스 실행
        if (shortest_index != -1) {
            PROCESS shortest_process = processes[readyQ_SJF.data[shortest_index] - 1];
            for (int i = 0; i < shortest_process.cpu_burst; i++) {
                second[sec_index] = shortest_process.pid;
                //printf("sec = %d | processing... %d \n", sec_index, shortest_process.pid);
                sec_index++;
            }
            //printf("shortest index = %d\n", shortest_index);
            
            // 실행 완료된 프로세스 레디 큐에서 제거
            for (int i = shortest_index; i < readyQ_SJF.size - 1; i++) {
                readyQ_SJF.data[i] = readyQ_SJF.data[i + 1];
            }
            readyQ_SJF.size--;
        }
        else {
            sec_index++;
            //printf("sec = %d \n", sec_index);
        }
    }
    end_second = sec_index;
    SCHEDULED_RESULT result = {second, end_second};
    return result;
}

SCHEDULED_RESULT Priority() {
    //printf("Priority is started!\n");

    //second 배열 선언 
    int *second = (int*)malloc(MAX_SECOND * sizeof(int)); //second 배열의 크기는 전체 프로세스 처리 시간
    int sec_index = 0;
    int end_second;

    while(!Is_empty(&readyQ_prt)) {
        //printf("[while]\n");
        int min_priority_index = -1;
        int min_priority = 100; // inf로 초기화하는 것과 같음

        for (int i = 0; i < readyQ_prt.size; i++) {
            PROCESS p = processes[readyQ_prt.data[i] - 1]; 
            if ((p.priority < min_priority) && (p.arrival_time <= sec_index)) {
                min_priority = p.priority;
                min_priority_index = i;
            }
        }
        //printf("min_priority process = %d, priority = %d\n", readyQ_prt.data[min_priority_index], min_priority);
        // 가장 작은 우선순위의 프로세스 실행
        if (min_priority_index != -1) {
            PROCESS min_priority_process = processes[readyQ_prt.data[min_priority_index] - 1];
            for (int i = 0; i < min_priority_process.cpu_burst; i++) {
                second[sec_index] = min_priority_process.pid;
                //printf("sec = %d | processing... %d \n", sec_index, min_priority_process.pid);
                sec_index++;
            }
            
            // 실행 완료된 프로세스 레디 큐에서 제거
            for (int i = min_priority_index; i < readyQ_prt.size - 1; i++) {
                readyQ_prt.data[i] = readyQ_prt.data[i + 1];
            }
            readyQ_prt.size--;
        }
        else {
            sec_index++;
            //printf("sec = %d \n", sec_index);
        }
    }
    end_second = sec_index;
    SCHEDULED_RESULT result = {second, end_second};
    return result;
}

SCHEDULED_RESULT RR() { //error
    //프로세스 배열에 접근/수정해야 하므로 사용할 배열을 복사
    PROCESS copied_processes[MAX_PROCESS];
    for (int i = 0; i < MAX_PROCESS; i++) {
        copied_processes[i].pid = processes[i].pid;
        copied_processes[i].cpu_burst = processes[i].cpu_burst; 
        copied_processes[i].io_burst = processes[i].io_burst;
        copied_processes[i].arrival_time = processes[i].arrival_time; 
        copied_processes[i].priority = processes[i].priority; 
    }
    
    //readyQ의 첫번째 원소를 프로세스로 선택
    PROCESS current_process = copied_processes[readyQ_RR.data[0] - 1];
    
    int *second = (int*)malloc(MAX_SECOND * sizeof(int)); //second 배열의 크기는 전체 프로세스 처리 시간
    int sec_index = 0;
    int process_index = 0;
    int current_cpu_burst = current_process.cpu_burst; //현재 프로세스의 cpuburst를 따로 분리해 현재 burst로 선언
    int end_second; // 마지막 프로세스가 끝나는 시간
    int time_quantum = TQ;
    
    //0초부터 첫 번째 프로세스 도착 시간까지의 기록
    if (current_process.arrival_time > 0) { 
        for (int i = 0; i < current_process.arrival_time; i++) {
            second[i] = 0; 
        }
        sec_index = current_process.arrival_time;
    }

    while(!Is_empty(&readyQ_RR)) { //ready queue가 빌 때까지 반복
        //현재 프로세스의 도착 시간이 second보다 큰 경우 : 실행할 프로세스가 없음 => 해당 second에 0 기록
        if (current_process.arrival_time > sec_index) {
            second[sec_index] = 0;
            sec_index++;
            
            continue;
        }
        //현재 burst의 값이 0 : 실행 완료 => 디큐하고 다음 프로세스 선택, 현재 burst 재설정, tq 리셋
        if (current_cpu_burst == 0) { // Process 실행 완료
            if(current_process.pid != 0) Dequeue(&readyQ_RR);
            
            if(!Is_empty(&readyQ_RR)){
                if (process_index == MAX_PROCESS){ 
                    process_index = 0;
                    Enqueue(&readyQ_RR, current_process.pid);
                }
                else process_index++;
                current_process = copied_processes[readyQ_RR.data[process_index]-1];
                current_cpu_burst = current_process.cpu_burst;
                time_quantum = TQ;
            }
            else {
                end_second = sec_index;
                break;
            }
        }
        else { //현재 burst의 값이 0 아니면 실행중 
            if (time_quantum > 0) { //time quantum 남음 : 실행중 => tq--, 현재 burst--
                second[sec_index] = current_process.pid;
                current_cpu_burst--;
                sec_index++;
                time_quantum--;
            }
            else { //time quantum이 0 : 강제 교체 
            // 다음 프로세스 선택 => 현재 프로세스 dequeue 후 readyQ 마지막에 enqueue 
                copied_processes[readyQ_RR.data[process_index]-1].cpu_burst = current_cpu_burst; //지금까지 수행한 cpu burst update
                
                if(current_process.pid != 0){
                    Dequeue(&readyQ_RR);
                    Enqueue(&readyQ_RR, current_process.pid);
                }
                
                if (process_index == MAX_PROCESS) { 
                    process_index = 0;
                    Enqueue(&readyQ_RR, current_process.pid);
                }
                else process_index++;
                
                current_process = copied_processes[readyQ_RR.data[process_index]-1];
                current_cpu_burst = current_process.cpu_burst;
                time_quantum = TQ; // tq 리셋
            }
        }
    }
    SCHEDULED_RESULT result = {second, end_second};
    return result;
}

SCHEDULED_RESULT Preemptive_SJF() { // error
    printf("Preemptive_SJF is started!\n");

    //second 배열 선언 
    int *second = (int*)malloc(MAX_SECOND * sizeof(int)); //second 배열의 크기는 전체 프로세스 처리 시간
    int sec_index = 0;
    int end_second;

    while(readyQ_prmtSJF.size > 0) {
        printf("[while]\n");
        int shortest_index = -1; 
        int shortest_burst = 100; // inf로 초기화하는 것과 같음
        
        // 1. 현재 시간과 arrv time을 비교해 레디큐에서 작거나 같은 프로세스들만 고려
        for (int i = 0; i < readyQ_prmtSJF.size; i++) {
            PROCESS p = processes[readyQ_prmtSJF.data[i]-1];
            if (p.arrival_time <= sec_index && p.cpu_burst < shortest_burst) {
                // 2. 그 중에서 CPU burst가 제일 작은 프로세스를 선택
                shortest_burst = p.cpu_burst;
                shortest_index = i;
            }
        }
        
        if (shortest_index != -1) {
            PROCESS shortest_process = processes[readyQ_prmtSJF.data[shortest_index]-1];

            // 3. second 배열에 해당 pid 입력 후 시간++, burst--
            second[sec_index] = shortest_process.pid;
            sec_index++;
            shortest_process.cpu_burst--;
            processes[readyQ_prmtSJF.data[shortest_index]-1].cpu_burst = shortest_process.cpu_burst;
            
            printf("shortest process = %d, burst = %d\n", shortest_process.pid, shortest_burst);
            // 6. burst = 0이면 실행 완료. readyQ에서 제거
            if (shortest_process.cpu_burst == 0) {
                for (int i = shortest_index; i < readyQ_prmtSJF.size -1; i++) {
                    readyQ_prmtSJF.data[i] = readyQ_prmtSJF.data[i+1];
                }
                readyQ_prmtSJF.size--;
                printf("Dequeued! \n");
            }
        } else { //실행할 프로세스가 없을 때
            second[sec_index] = 0;
            sec_index++;
        }
        printf("sec_index = %d\n", sec_index);
        //sleep(5);
        
    }
    end_second = sec_index;
    SCHEDULED_RESULT result = {second, end_second};
    return result;
}

SCHEDULED_RESULT Preemptive_Priority() {

}

void Evaluation(SCHEDULED_RESULT *result);

void Schedule() { // schedule all algorithm and print gantt chart
    printf("---------------------------\n\n");
    
    // FCFS
    SCHEDULED_RESULT FCFS_result = FCFS();
    int end = FCFS_result.end_process;
    printf("Total Process time = %d\n\n", end);
    printf("FCFS Second =\t  { ");
    for (int i = 0; i < end; i++) {
        printf("%d ", FCFS_result.second[i]);
    }
    printf("}\n\n");
    Evaluation(&FCFS_result); 
    free(FCFS_result.second);
    printf("---------------------------\n\n");
    
    // SJF 확인
    SCHEDULED_RESULT SJF_result = SJF();
    printf("SJF Second =\t  { ");
    for (int i = 0; i < end; i++) {
        printf("%d ", SJF_result.second[i]);
    }
    printf("}\n\n");
    Evaluation(&SJF_result);
    free(SJF_result.second);
    printf("---------------------------\n\n");

    // Priority 확인
    SCHEDULED_RESULT priority_result = Priority();
    printf("Priority Second = { ");
    for (int i = 0; i < end; i++) {
        printf("%d ", priority_result.second[i]);
    }
    printf("}\n\n");
    Evaluation(&priority_result);
    free(priority_result.second);
    printf("---------------------------\n\n");

    // RR 확인
    SCHEDULED_RESULT RR_result = RR();
    printf("RR Second =\t  { ");
    for (int i = 0; i < end; i++) {
        printf("%d ", RR_result.second[i]);
    }
    printf("}\n\n");
    free(RR_result.second);
    
    /*
    SCHEDULED_RESULT PrmtSJF_result = Preemptive_SJF();

    // Preemptive_SJF second 확인
    printf("Preemptive_SJF Second = {");
    for (int i = 0; i < end; i++) {
        printf("%d ", PrmtSJF_result.second[i]);
    }
    printf("}\n\n");
    printf("Total Process time = %d\n\n", PrmtSJF_result.end_process); //temp
    free(PrmtSJF_result.second);

    Preemptive_Priority();
    */
    
}

//스케줄링 결과 second 배열을 이용해 average waiting time, average turnaround time 구하기
void Evaluation(SCHEDULED_RESULT *result) { 
    int total_waiting_time = 0;
    int total_turnaround_time = 0;

    for (int i = 0; i < MAX_PROCESS; i++) {
        int waiting_time = 0;
        int turnaround_time = 0;

        PROCESS now_process = processes[i];
        int cnt = 0;

        for (int sec_index = 0; sec_index <= result->end_process; sec_index++) {
            
            int current_process = result->second[sec_index];
            if (cnt == now_process.cpu_burst) { //프로세스 처리 완료
                turnaround_time = sec_index - now_process.arrival_time;
                break;
            }
            if (sec_index != 0) {
                if ((current_process == now_process.pid) && (result->second[sec_index-1] != now_process.pid)) {
                    waiting_time = sec_index - now_process.arrival_time;
                }
            }
            if (current_process == now_process.pid) {
                cnt++;
                if (sec_index == 0) waiting_time = 0;
            }
        }
        total_waiting_time += waiting_time;
        total_turnaround_time += turnaround_time;
    }

    printf("average waiting time = %.2f\n", (float)total_waiting_time/MAX_PROCESS);
    printf("average turnaround time = %.2f\n\n", (float)total_turnaround_time/MAX_PROCESS);
}



int main() {
    Create_Process();
    
    printf("Process Check\n");
    for (int i=0; i<MAX_PROCESS; i++) {
        printf("PID : %d\n", processes[i].pid);
        printf("CPU burst time : %d\n", processes[i].cpu_burst);
        //printf("I/O burst time : %d\n", processes[i].io_burst);
        printf("Arrival time : %d\n", processes[i].arrival_time);
        printf("Priority = %d\n\n", processes[i].priority);
    }

    Config();
    
    printf("Sorted processes = { ");
    for (int i = 0; i < MAX_PROCESS; i++) {
        printf("%d ", processes[i].pid);
    }
    printf("}\n\n");
    printf("Queue Check\nReadyQ = { ");
    for (int i = 0; i<MAX_QUEUE; i++) {
        printf("%d ", readyQ.data[i]);
    }
    printf("}\n");
    
    printf("\nScheduling is started!\n");
    Schedule();
    printf("Scheduling is done!\n");
    
    return 0;
}

