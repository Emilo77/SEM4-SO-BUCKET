/* Przykładowy test szeregowania.
 * Test jest parametryzowany liczbami całkowitymi `x` oraz `y`.
 * W teście menedżer tworzy pięć pomocniczych procesów A, B, C, D, E, gdzie:
 *   -- proces A należy do kubełka nr 1 i wykonuje jako swoją pracę
 *         `x` iteracji petli,
 *   -- procesy B, C, D, E należą do kubełka nr 2 i każdy z nich wykonuje
 *          jako swoją pracę po `y` iteracji pętli.
 *
 * Po zakończeniu pracy pomocniczy proces się kończy, a menedżer odnotowuje
 *   kolejność, w jakiej procesy kończyły pracę.
 *
 * W takim modelu proces A powinien wykonywać się czterokrotnie szybciej niż
 *   procesy B, C, D, E.
 * Wobec tego oczekujemy, że:
 *   -- dla parametrów `x = BASE_NUM_ITERS * 15` oraz `y = BASE_NUM_ITERS / 2`,
 *          procesy B, C, D, E zakończą się przed procesem A,
 *   -- dla parametrow `x = BASE_NUM_ITERS * 3` oraz `y = BASE_NUM_ITERS * 2`,
 *          procesy B, C, D, E zakończą się po procesie A.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define BASE_NUM_ITERS ((uint64_t)(1500) * 1000 * 1000)
#define SMALL_NUM_ITERS ((uint64_t)(50) * 1000)
#define NUM_JOBS 5

/* Ta funkcja wykonuje pracę w procesach pomocniczych.
 * Praca jest modelowana poprzez wykonanie `num_iters` iteracji prostej pętli.
 */
volatile uint64_t work_result;

void work(uint64_t num_iters) {
  uint64_t a = 1;
  while (num_iters--) {
    a *= 3;
  }
  work_result = a;
}

/* To jest główna funkcja wykonywana przez proces pomocniczy. */
void runner(uint64_t num_iters) {
  /* Mały trik polepszający deterministyczność działania testu:
   *   uruchamiamy pętlę wykonującą bardzo małą ilość pracy
   *   i czekamy sekundę za pomoca funkcji `sleep`.
   */
  work(SMALL_NUM_ITERS);
  sleep(1);

  /* Teraz wykonujemy główną pracę i kończymy proces. */
  work(num_iters);
  /* Użycie `_exit` zamiast `exit` wydaje się być bardziej deterministyczne:
   *   druga funkcja czasami musi przywołać do pamięci fizycznej
   *   część kodu biblioteki standardowej, co zużywa kilka kwantów czasu.
   */
  _exit(0);
}

/* To jest funkcja przeprowadzająca test. Przyjmuje parametry `x` oraz `y`
 *   zgodnie z opisem w nagłówku pliku.
 * Funkcja zapisuje w tablicy `final_order` kolejność kończenia się procesów
 *   (numeracja od 0).
 */
void run_test(uint64_t x, uint64_t y, int *final_order) {
  uint64_t work_sizes[NUM_JOBS] = {x, y, y, y, y};
  int bucket_nrs[NUM_JOBS] = {1, 2, 2, 2, 2};
  int pids[NUM_JOBS];

  /* Tworzymy NUM_JOBS pomocniczych procesów, ustawiamy im numery kubełków
   * i zlecamy im pracę.
   */
  for (int i = 0; i < NUM_JOBS; i++) {
    pids[i] = fork();
    if (pids[i] < 0) {
      perror("fork");
      exit(1);
    } else if (pids[i] == 0) {
      /* Proces-dziecko. */
      set_bucket(bucket_nrs[i]);
      runner(work_sizes[i]);
      __builtin_unreachable();
    }
    /* Proces-ojciec kontynuuje iterowanie petli. */
  }

  /* Czekamy na zakończenie procesów i zapisujemy porządek, w jakim
   * procesy się zakończyły.
   */
  for (int i = 0; i < NUM_JOBS; i++) {
    int wstatus;
    int child_pid = wait(&wstatus);
    if (child_pid == -1) {
      perror("wait");
      exit(1);
    }

    final_order[i] = -1;
    for (int j = 0; j < NUM_JOBS; j++) {
      if (child_pid == pids[j]) {
        final_order[i] = j;
        printf("%c ", (char)('A' + j));
        fflush(stdout);
        break;
      }
    }
    assert(final_order[i] != -1);
  }
}

int main(void) {
  int final_order[NUM_JOBS];

  /* Podtest 1: proces A wykonuje wielokrotnie więcej pracy niż procesy B, C, D, E.
   * Wymagamy, aby proces A zakończył się jako ostatni.
   */
  printf("Podtest 1: ");
  fflush(stdout);
  run_test(BASE_NUM_ITERS * 15, BASE_NUM_ITERS / 2, final_order);
  printf(" => %s\n", (final_order[NUM_JOBS - 1] == 0) ? "OK" : "WRONG");

  /* Podtest 2: proces A wykonuje 1,5 razy więcej pracy niż procesy B, C, D, E.
   * Wymagamy, aby proces A zakończył się jako pierwszy.
   */
  printf("Podtest 2: ");
  fflush(stdout);
  run_test(BASE_NUM_ITERS * 3, BASE_NUM_ITERS * 2, final_order);
  printf(" => %s\n", (final_order[0] == 0) ? "OK" : "WRONG");

  return 0;
}
