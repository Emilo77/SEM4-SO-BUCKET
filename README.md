## BUCKETS
Celem zadania jest stworzenie strategii szeregowania procesów użytkownika.

Procesy użytkownika mają priorytet ustawiony na stałe na `BUCKET_Q` i umieszczone są w *kubełkach* o indeksach będących liczbami całkowitymi od `0` do `NR_BUCKETS - 1`. Domyślnie każdy proces użytkownika znajduje się w kubełku o numerze 0. Proces użytkownika może trafić do innego kubełka w kilku przypadkach:

- Wywołanie systemowe `set_bucket(int bucket_nr)` przenosi wywołujący proces do kubełka o indeksie `bucket_nr`.
- Gdy proces wywoła funkcję `fork`, nowo powstały proces dziedziczy po swoim rodzicu jego numer kubełka.

### Szeregowanie

System umieszcza wszystkie procesy należące do jednego kubełka w jednej kolejce procesów. W ramach każdej kolejki system wykorzystuje algorytm planowania rotacyjnego (ang. *round-robin*). Zasada wybierania kolejki jest następująca: wszystkie kubełki umieszczone są w cyklu, w kolejności 0, 1, ..., `NR_BUCKETS - 1`. Jeśli ostatni kwant czasu otrzymał proces wybrany z kubełka o indeksie `k`, to następny kwant czasu otrzyma proces z pierwszego niepustego kubełka, który występuje w cyklu po kubełku `k`.

Algorytm szeregowania procesów systemowych pozostaje bez zmian. Należy jednak zapewnić, by procesom tym nie został nadany priorytet `BUCKET_Q`.

### Przykład

Rozważmy scenariusz, w którym procesy użytkownika nie blokują, nie zmieniają kubełków ani nie kończą swojej pracy. Rozważmy 7 procesów, które należą do trzech kubełków o indeksach 0, 1, 2. Początkowa zawartość kubełków jest następująca:

```asm
0: p1
1: p2 p3
2: p4 p5 p6 p7
```

W tym scenariuszu procesy będą otrzymywały kwanty czasu w następującej kolejności:
```asm
p1 p2 p4 p1 p3 p5 p1 p2 p6 p1 p3 p7 ...
```

### Implementacja

Implementacja powinna zawierać:
- definicje stałych `BUCKET_Q = 8` oraz `NR_BUCKETS = 10`,
- nową funkcję systemową `int set_bucket(int bucket_nr)`,
- komentarz `/* so_2022 */` bezpośrednio za nagłówkiem każdej funkcji, która została dodana lub zmieniona.

Jeśli wartość argumentu `bucket_nr` jest z przedziału od `0` do `NR_BUCKETS - 1`, to procesowi zostaje przypisany kubełek o numerze `bucket_nr`, a funkcja zwraca `0`. W przeciwnym przypadku funkcja zwraca `-1`, a zmienna `errno` przyjmuje wartość `EINVAL`. Natomiast gdy funkcja `set_bucket` zostanie wywołana przez proces systemowy, funkcja powinna zwrócić `-1`, a zmienna `errno` powinna przyjąć wartość `EPERM`.

Dopuszczamy zmiany w katalogach:

- `/usr/src/lib/libc/misc/`,
- `/usr/src/minix/kernel/`,
- `/usr/src/minix/kernel/system/`,
- `/usr/src/minix/lib/libsys/`,
- `/usr/src/minix/servers/pm/`,
- `/usr/src/minix/servers/sched/`

i dodatkowo w poniższych plikach nagłówkowych:

- `/usr/src/include/unistd.h`,
- `/usr/src/minix/include/minix/callnr.h`,
- `/usr/src/minix/include/minix/com.h`,
- `/usr/src/minix/include/minix/config.h`,
- `/usr/src/minix/include/minix/ipc.h`,
- `/usr/src/minix/include/minix/syslib.h`.

### Wskazówki

- Przypominamy, że wstawianie do kolejki procesów gotowych wykonuje mikrojądro (`/usr/src/minix/kernel/proc.c`). Natomiast o przydzielaniu kwantu i priorytetu procesowi decyduje serwer `sched`.

- Nie trzeba pisać nowego serwera szeregującego. Można zmodyfikować domyślny serwer `sched`.

- Należy dopilnować tego, by funkcja systemowa `nice` nie zmieniała priorytetów procesów użytkownika.

- Aby nowy algorytm szeregowania zaczął działać, należy wykonać `make; make install` w katalogu `/usr/src/minix/servers/sched` oraz w innych katalogach zawierających zmiany. Następnie trzeba zbudować nowy obraz jądra, czyli wykonać `make do-hdboot` w katalogu `/usr/src/releasetools` i ponownie uruchomić system. Gdyby obraz nie chciał się załadować lub wystąpił poważny błąd (*kernel panic*), należy przy starcie systemu wybrać opcję `6`, która załaduje oryginalne jądro.

- Zakładamy, że procesami użytkownika są te procesy, którymi zarządza serwer szeregujący `sched`. Natomiast procesy systemowe to te, które są szeregowane bezpośrednio przez jądro, bez pośrednictwa serwera szeregującego.

- Jądro oraz serwery mogą wypisywać dane diagnostyczne za pomocą funkcji `printf`, co może być przydatne w celu debugowania łatki. Domyślnie dane są wypisywane w oknie terminala. W przypadku potrzeby wypisania większej ilości danych polecamy skorzystać z wirtualnego portu szeregowego. W tym celu należy uruchomić emulator QEMU z dodatkową opcją `-serial file:serial.txt`. Następnie przy starcie MINIX-a należy wybrać opcję 4 i zmodyfikować domyślną opcję 2 poprzez dopisanie dodatkowej flagi `cttyline=0`. W wyniku tych modyfikacji dane diagnostyczne jądra i serwerów MINIX-a będą wypisywane do pliku `serial.txt` w komputerze hosta.

### Ocena rozwiązania

Poniżej przyjmujemy, że `ab123456` oznacza identyfikator studenta rozwiązującego zadanie. Należy przygotować łatkę (ang. *patch*) ze zmianami w sposób opisany w treści zadania 3. Rozwiązanie w postaci łatki `ab123456.patch` należy umieścić w Moodle. Opcjonalnie można dołączyć plik `README`.

W celu skompilowania i uruchomienia systemu z nową wersją szeregowania wykonane będą następujące polecenia:

```console
cd /
patch -t -p1 < ab123456.patch
cd /usr/src; make includes
cd /usr/src/minix/kernel; make && make install
cd /usr/src/minix/lib/libsys; make && make install
cd /usr/src/minix/servers/sched; make && make install
cd /usr/src/minix/servers/pm; make && make install
cd /usr/src/lib/libc; make && make install
cd /usr/src/releasetools; make do-hdboot
```

Następnie nowo zbudowany system zostanie przetestowany na serii testów automatycznych. Jeden z tych testów został dołączony do treści zadania. Ostateczny wynik rozwiązania będzie zależał od: stabilności systemu, poprawności i efektywności działania na testach automatycznych oraz stylu kodowania.

Prosimy pamiętać o dodaniu odpowiednich komentarzy, ponieważ lista zmienionych funkcji uzyskana za pomocą polecenia `grep -r so_2022 /usr/src/` może mieć wpływ na ocenę zadania. Wystarczy, że każda funkcja pojawi się na liście tylko raz, więc nie potrzeba umieszczać komentarzy w plikach nagłówkowych.

Uwaga: nie przyznajemy punktów za rozwiązanie, w którym łatka nie nakłada się poprawnie, które nie kompiluje się lub powoduje *kernel panic* podczas uruchamiania.

### Pytania

Pytania do tego zadania należy kierować na adres `marek.sokolowski@mimuw.edu.pl`. Odpowiedzi na często zadawane pytania będą pojawiać się na forum w Moodle.

