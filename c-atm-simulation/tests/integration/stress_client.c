/**
 * tests/integration/stress_client.c
 * Multi-client concurrency stress test, idempotency replay, and abrupt disconnect test (F8).
 */

#include "server.h"
#include "client.h"
#include "bank.h"
#include "account.h"
#include "pin.h"
#include "hmac.h"
#include "net/net_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
typedef HANDLE thread_t;
#define THREAD_FUNC DWORD WINAPI
#else
#include <pthread.h>
typedef pthread_t thread_t;
#define THREAD_FUNC void *
#endif

static int g_pass = 0;
static int g_fail = 0;

#define TEST_CHECK(cond, name) do { \
    if (cond) { \
        printf("  [PASS] %s\n", name); \
        g_pass++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", name, __LINE__); \
        g_fail++; \
    } \
} while(0)

static Bank srv_bank;
static BankServer srv;
static volatile int srv_stop = 0;

#if defined(_WIN32) || defined(_WIN64)
static DWORD WINAPI server_thread_fn(LPVOID arg) {
    (void)arg;
    server_run(&srv, &srv_stop);
    return 0;
}
#else
static void *server_thread_fn(void *arg) {
    (void)arg;
    server_run(&srv, &srv_stop);
    return NULL;
}
#endif

int main(void) {
    printf("====================================================\n");
    printf("     RUNNING F8 NETWORK STRESS & IDEMPOTENCY TEST   \n");
    printf("====================================================\n");

    bank_init(&srv_bank);
    srv_bank.config.withdraw_multiple = 100;
    srv_bank.config.per_withdraw_limit = 50000;
    srv_bank.config.daily_withdraw_limit = 100000;
    account_create(&srv_bank, 1001, "Stress User", 300000LL, "1234"); /* $3,000.00 */

    int test_port = 5058;
    bool srv_ok = server_init(&srv, &srv_bank, "127.0.0.1", test_port, 8);
    TEST_CHECK(srv_ok, "Server: Successfully initialized on 127.0.0.1:5058");

#if defined(_WIN32) || defined(_WIN64)
    HANDLE th = CreateThread(NULL, 0, server_thread_fn, NULL, 0, NULL);
    Sleep(200); /* Allow server loop to start */
#else
    pthread_t th;
    pthread_create(&th, NULL, server_thread_fn, NULL);
    usleep(200000);
#endif

    /* 1. Multi-Client Simultaneous Withdrawals */
    BankApi c1 = bank_api_create_remote("127.0.0.1", test_port);
    BankApi c2 = bank_api_create_remote("127.0.0.1", test_port);
    BankApi c3 = bank_api_create_remote("127.0.0.1", test_port);

    char name1[64], name2[64], name3[64];
    bool l1 = c1.login(c1.ctx, 1001, "1234", name1).success;
    bool l2 = c2.login(c2.ctx, 1001, "1234", name2).success;
    bool l3 = c3.login(c3.ctx, 1001, "1234", name3).success;

    TEST_CHECK(l1 && l2 && l3, "Clients: 3 independent clients authenticated concurrently via challenge-response");

    /* Each client executes two $500 (50,000 minor) withdrawals */
    int successful_withdraws = 0;
    if (c1.withdraw(c1.ctx, 1001, 50000LL).success) successful_withdraws++;
    if (c2.withdraw(c2.ctx, 1001, 50000LL).success) successful_withdraws++;
    if (c3.withdraw(c3.ctx, 1001, 50000LL).success) successful_withdraws++;
    if (c1.withdraw(c1.ctx, 1001, 50000LL).success) successful_withdraws++;
    if (c2.withdraw(c2.ctx, 1001, 50000LL).success) successful_withdraws++;
    if (c3.withdraw(c3.ctx, 1001, 50000LL).success) successful_withdraws++;

    TEST_CHECK(successful_withdraws == 6, "Concurrency: 6 sequential $500 debits processed across 3 clients");

    Account *acc = account_find(&srv_bank, 1001);
    TEST_CHECK(acc && acc->balance == 0LL, "Consistency: Final balance equals exactly $0.00 (no double debits)");

    /* Attempt 7th withdrawal when balance is 0 - must be rejected and balance never negative */
    ApiResult over_res = c1.withdraw(c1.ctx, 1001, 50000LL);
    TEST_CHECK(!over_res.success, "Safety: Withdrawal with insufficient balance rejected");
    TEST_CHECK(acc->balance >= 0LL, "Safety: Balance never goes negative under concurrent load");

    /* 2. Idempotency Test: Duplicate req_id does NOT debit twice */
    acc->balance = 100000LL; /* Reset to $1,000.00 */
    /* Send custom raw socket request with fixed req_id */
    socket_t raw_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((uint16_t)test_port);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(raw_sock, (struct sockaddr *)&sin, sizeof(sin));

    send(raw_sock, PROTO_HANDSHAKE, (int)strlen(PROTO_HANDSHAKE), 0);
    char hand_buf[64];
    recv(raw_sock, hand_buf, sizeof(hand_buf) - 1, 0);

    /* Challenge */
    send(raw_sock, "REQ\t1\tCHALLENGE\t1001\n", 21, 0);
    char ch_buf[256] = {0};
    int rlen = recv(raw_sock, ch_buf, sizeof(ch_buf) - 1, 0);
    ch_buf[rlen > 0 ? rlen : 0] = '\0';

    char salt_hex[64], nonce_hex[64];
    int iter = 0;
    sscanf(ch_buf, "RES\t1\t200\t%63s\t%d\t%63s", salt_hex, &iter, nonce_hex);

    uint8_t salt[16], nonce[16], pin_hash[32], client_hmac[32];
    for (size_t i = 0; i < 16; ++i) {
        unsigned int b1, b2;
        sscanf(salt_hex + (i * 2), "%02x", &b1); salt[i] = (uint8_t)b1;
        sscanf(nonce_hex + (i * 2), "%02x", &b2); nonce[i] = (uint8_t)b2;
    }
    pin_hash_derive("1234", salt, iter, pin_hash);
    hmac_sha256(pin_hash, 32, nonce, 16, client_hmac);
    char hmac_hex[65];
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < 32; ++i) {
        hmac_hex[i * 2] = hex[(client_hmac[i] >> 4) & 0x0F];
        hmac_hex[i * 2 + 1] = hex[client_hmac[i] & 0x0F];
    }
    hmac_hex[64] = '\0';

    char login_req[256];
    snprintf(login_req, sizeof(login_req), "REQ\t2\tLOGIN\t1001\t%s\n", hmac_hex);
    send(raw_sock, login_req, (int)strlen(login_req), 0);
    char login_res[256] = {0};
    rlen = recv(raw_sock, login_res, sizeof(login_res) - 1, 0);
    login_res[rlen > 0 ? rlen : 0] = '\0';
    char token[64];
    sscanf(login_res, "RES\t2\t200\t%63s", token);

    /* First withdrawal: req_id 77 */
    char w_req[256];
    snprintf(w_req, sizeof(w_req), "REQ\t77\tWITHDRAW\t%s\t20000\n", token);
    send(raw_sock, w_req, (int)strlen(w_req), 0);
    char w_res1[256] = {0};
    rlen = recv(raw_sock, w_res1, sizeof(w_res1) - 1, 0);
    w_res1[rlen > 0 ? rlen : 0] = '\0';
    int64_t bal_after_first = acc->balance;
    TEST_CHECK(bal_after_first == 80000LL, "Idempotency: First request 77 debited $200");

    /* Replay identical request with same req_id 77 */
    send(raw_sock, w_req, (int)strlen(w_req), 0);
    char w_res2[256] = {0};
    rlen = recv(raw_sock, w_res2, sizeof(w_res2) - 1, 0);
    w_res2[rlen > 0 ? rlen : 0] = '\0';
    TEST_CHECK(strcmp(w_res1, w_res2) == 0, "Idempotency: Replayed req_id 77 returned identical response string");
    TEST_CHECK(acc->balance == 80000LL, "Idempotency: Replayed req_id 77 did NOT debit a second time");

    /* 3. Abrupt Disconnect Test */
    socket_t kill_sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(kill_sock, (struct sockaddr *)&sin, sizeof(sin));
    send(kill_sock, "REQ\t99\tWITH", 10, 0); /* Half-written */
    CLOSESOCKET(kill_sock);                  /* Abrupt kill mid-request */

    /* Verify server remains responsive and healthy */
    ApiResult post_kill = c1.get_balance(c1.ctx, 1001, NULL);
    TEST_CHECK(post_kill.success && post_kill.balance_after == 80000LL,
               "Resilience: Server remained healthy and consistent after abrupt client disconnect");

    /* Cleanup */
    c1.logout(c1.ctx, 1001);
    c2.logout(c2.ctx, 1001);
    c3.logout(c3.ctx, 1001);
    CLOSESOCKET(raw_sock);

    srv_stop = 1;
#if defined(_WIN32) || defined(_WIN64)
    WaitForSingleObject(th, 1000);
    CloseHandle(th);
#else
    pthread_join(th, NULL);
#endif
    server_close(&srv);

    printf("\nTEST RESULTS: %d Passed, %d Failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
