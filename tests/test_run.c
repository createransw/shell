#include <check.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include "../src/run.h"

bool cmp_files(const char *file1, const char *file2) {
    FILE *f1 = fopen(file1, "r");
    FILE *f2 = fopen(file2, "r");

    if (f1 == NULL || f2 == NULL) {
        if (f1) {
            fclose(f1);
        }
        if (f2) {
            fclose(f2);
        }

        return false;
    }

    int c1 = fgetc(f1);
    int c2 = fgetc(f2);


    while (c1 != EOF) {
        fputc(c1, stdout);
        if (c1 != c2) {
            fclose(f1);
            fclose(f2);
            return false;
        }

        c1 = fgetc(f1);
        c2 = fgetc(f2);
    }

    fclose(f1);
    fclose(f2);
    return true;
}

START_TEST(test_output) {
    ck_assert(cmp_files("./tests/data/result", "./tests/data/result_r"));
}
END_TEST

START_TEST(test_text) {
    ck_assert(cmp_files("./tests/data/file.out", "./tests/data/file_r.out"));
}
END_TEST

START_TEST(test_who) {
    ck_assert(cmp_files("./tests/data/who_am_i.txt", "./tests/data/who_am_i_r.txt"));
}
END_TEST

START_TEST(test_dir) {
    ck_assert(cmp_files("./tests/data/current_dir.txt", "./tests/data/current_dir_r.txt"));
}
END_TEST

START_TEST(test_pwd) {
    ck_assert(cmp_files("./tests/data/fpwd", "./tests/data/fpwd_r"));
}
END_TEST

START_TEST(test_error) {
    ck_assert(cmp_files("./tests/data/error", "./tests/data/error_r"));
}
END_TEST


Suite* shell_suit() {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Shell");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_output);
    tcase_add_test(tc_core, test_text);
    tcase_add_test(tc_core, test_who);
    tcase_add_test(tc_core, test_dir);
    tcase_add_test(tc_core, test_pwd);
    tcase_add_test(tc_core, test_error);

    suite_add_tcase(s, tc_core);

    return s;
}

int main() {
    int number_failed = 0;
    Suite *s;
    SRunner *sr;

    s = shell_suit();
    sr = srunner_create(s);

    int process = fork();

    FILE *f;
    FILE *f_e;
    if (process == 0) {
        freopen("./tests/data/commands_r", "r", stdin);
    } else {
        freopen("./tests/data/commands", "r", stdin);
    }

    int out = dup(fileno(stdout));
    if (process == 0) {
        freopen("./tests/data/result_r", "w", stdout);
        freopen("./tests/data/error_r", "w", stderr);
    } else {
        freopen("./tests/data/result", "w", stdout);
        freopen("./tests/data/error", "w", stderr);
    }
    if (process == 0) {
        execvp("bash", NULL);
    }
    waitpid(process, NULL, 0);
    process = fork();
    if (process == 0) {
        dialog();
    }
    waitpid(process, NULL, 0);
    dup2(out, fileno(stdout));

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}
