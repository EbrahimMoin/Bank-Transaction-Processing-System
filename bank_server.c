#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>
#include <pthread.h>

#define QUEUE_NAME "/bank_queue"
#define MAX_MSG_SIZE 256
#define ACCOUNT_COUNT 5

typedef struct {
    int balance;    
} Account;

Account accounts[ACCOUNT_COUNT];
sem_t counter_sem;

void handle_message(char *message);

void* thread_handle_message(void* arg) {
    char* msg = (char*)arg;
    handle_message(msg);
    free(msg);
    return NULL;
}

void initialize_accounts() {
    for (int i = 0; i < ACCOUNT_COUNT; i++) {
        accounts[i].balance = 1000;
    }
}

void process_transaction(char *action, int account_index, int amount) {
    sem_wait(&counter_sem);

    if (strcmp(action, "deposit") == 0) {
        accounts[account_index].balance += amount;
        printf("[+] Deposited $%d into Account[%d]. New Balance: $%d\n", amount, account_index, accounts[account_index].balance);
    } else if (strcmp(action, "withdraw") == 0) {
        if (accounts[account_index].balance >= amount) {
            accounts[account_index].balance -= amount;
            printf("[+] Withdrew $%d from Account[%d]. New Balance: $%d\n", amount, account_index, accounts[account_index].balance);
        } else {
            printf("[!] Insufficient funds for Account[%d]\n", account_index);
        }
    }

    sem_post(&counter_sem);
}

void handle_message(char *message) {
    if (strcmp(message, "exit") == 0) {
        printf("[*] ATM requested shutdown. Exiting...\n");
        exit(0);
    }

    int account_index, amount;
    char action[20];

    sscanf(message, "%d %s %d", &account_index, action, &amount);

    if (account_index < 0 || account_index >= ACCOUNT_COUNT) {
        printf("[!] Invalid account index: %d\n", account_index);
        return;
    }

    if (strcmp(action, "view") == 0) {
        sem_wait(&counter_sem);
        printf("[*] Account[%d] Balance: $%d\n", account_index, accounts[account_index].balance);
        sem_post(&counter_sem);
    } else {
        process_transaction(action, account_index, amount);
    }
}

int main() {
    mqd_t mq;
    struct mq_attr attr;
    char buffer[MAX_MSG_SIZE];

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mq_unlink(QUEUE_NAME); 

    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq == -1) {
        perror("mq_open (bank)");
        exit(EXIT_FAILURE);
    }

    sem_init(&counter_sem, 0, 1);
    initialize_accounts();

    printf("[*] Bank Server Started.\n");

    while (1) {
        ssize_t bytes_read = mq_receive(mq, buffer, MAX_MSG_SIZE, NULL);
        if (bytes_read >= 0) {
            buffer[bytes_read] = '\0';
            // Allocate memory for the message to pass to the thread
            char* msg_copy = strdup(buffer);
            pthread_t tid;
            if (pthread_create(&tid, NULL, thread_handle_message, msg_copy) == 0) {
                pthread_detach(tid); // No need to join, auto-cleanup
            } else {
                perror("pthread_create");
                free(msg_copy);
            }
        } else {
            perror("mq_receive (bank)");
            sleep(1); // chill to avoid spam if something wrong
        }
    }

    mq_close(mq);
    mq_unlink(QUEUE_NAME);
    sem_destroy(&counter_sem);

    return 0;
}