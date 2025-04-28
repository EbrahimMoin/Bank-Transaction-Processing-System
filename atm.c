#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#define QUEUE_NAME "/bank_queue"
#define MAX_MSG_SIZE 256

void* atm_user_session(void* arg) {
    mqd_t mq;
    char buffer[MAX_MSG_SIZE];
    int choice, account_index, amount;
    int user_id = *(int*)arg;

    mq = mq_open(QUEUE_NAME, O_WRONLY);
    if (mq == -1) {
        perror("mq_open (atm)");
        pthread_exit(NULL);
    }

    printf("[*] User %d: ATM Connected to Bank.\n", user_id);

while (1) {
    printf("\n=== ATM Menu (User %d) ===\n", user_id);
    printf("1. Deposit\n");
    printf("2. Withdraw\n");
    printf("3. View Balance\n");
    printf("4. Transfer\n"); 
    printf("5. Exit\n");     
    printf("Select option: ");
    scanf("%d", &choice);

    if (choice == 5) { 
        printf("[*] User %d: ATM exiting...\n", user_id);
        break;
    }

    printf("Enter account index (0-4): ");
    scanf("%d", &account_index);

    if (account_index < 0 || account_index > 4) {
        printf("[!] Invalid account index.\n");
        continue;
    }

    if (choice == 1) {
        printf("Enter deposit amount: ");
        scanf("%d", &amount);
        snprintf(buffer, sizeof(buffer), "%d deposit %d", account_index, amount);
    } else if (choice == 2) {
        printf("Enter withdraw amount: ");
        scanf("%d", &amount);
        snprintf(buffer, sizeof(buffer), "%d withdraw %d", account_index, amount);
    } else if (choice == 3) {
        snprintf(buffer, sizeof(buffer), "%d view 0", account_index);
    } else if (choice == 4) { // Transfer option
        int to_account;
        printf("Enter target account index (0-4): ");
        scanf("%d", &to_account);
        if (to_account < 0 || to_account > 4 || to_account == account_index) {
            printf("[!] Invalid target account.\n");
            continue;
        }
        printf("Enter transfer amount: ");
        scanf("%d", &amount);
        snprintf(buffer, sizeof(buffer), "%d transfer %d %d", account_index, amount, to_account);
    } else {
        printf("[!] Invalid option.\n");
        continue;
    }

    if (mq_send(mq, buffer, strlen(buffer) + 1, 0) == -1) {
        perror("mq_send (atm)");
    }
}
    mq_close(mq);
    pthread_exit(NULL);
}

int main() {
    int user_id = getpid(); // Use process ID as a unique user ID
    atm_user_session(&user_id);
    return 0;
}
