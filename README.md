# FiveWordFiveLetters – C Version (Performance Fokus)

## Problemformulering

Lav et C-program, der kan finde **alle kombinationer af 5 ord**, hvor:

* Hvert ord har **præcis 5 bogstaver**.
* **Ingen bogstaver må gentages** på tværs af de 5 ord.
* Hvert bogstav må kun **optræde 1 gang**

Målet er **maksimal hastighed og effektiv hukommelsesudnyttelse**, uden fokus på grafisk brugerflade.

---

## Arbejdsgang (Refaktoreringstrin)

### **Step 1 – Indlæs data**

1. Opret en lille testfil med “perfekt” data (ét ord per linje, alle 5 bogstaver).
2. Indlæs filen i programmet (brug `fgets` eller `getline`).
3. Implementér en testfunktion, der sikrer korrekt indlæsning.
4. Udvid filen med “uperfekt” data (forkerte længder, specialtegn, duplikerede ord).
5. Refaktor indlæsning til at filtrere ugyldige ord.
6. Udvid testfunktionen til at tjekke fejlscenarier.

---

### **Step 2 – Beta data**

* Tilpas programmet til at arbejde med den store ordliste `words_beta.txt`.
* Sørg for at kun gyldige ord (5 bogstaver, A–Z) gemmes i hukommelsen.

---

### **Step 3 – Make it Right**

* Følg C-kodestandarder (navngivning, indrykning, konsistens).
* Brug fejlhåndtering (tjek filpointer, allokering, inputfejl).
---

### **Step 4 – Optimer datastruktur**

* Brug **bitmasker** til at repræsentere bogstaver (26-bit heltal, én bit pr. bogstav).
* Et ord kan lagres som et heltal i stedet for en streng → hurtigere sammenligning.
* Precompute bitmasker ved indlæsning.

---

### **Step 5 – Multi-threading**

* Parallelisér søgningen med **POSIX threads** (`pthread`).
* Del ordlisten i segmenter, så flere tråde arbejder samtidig.
* Brug thread-safe datastrukturer til at gemme resultater.

---

### **Step 6 – Maksimal hastighed**

* Undgå unødvendige kopier af data.
* Brug statiske arrays frem for heap-allokering, hvor muligt.
* Minimer brug af `printf` under beregninger.
* Eventuelt brug SIMD-instruktioner (AVX/SSE) hvis tilgængeligt.

---

## Ekstra funktionalitet

* Tillad ændring af antal ord og antal bogstaver via kommandolinjeargumenter.
* Mulighed for at gemme resultater i fil (`-o output.txt`).
* Statusudskrift til konsol (antal kombinationer fundet, tid brugt).

---

## Performance-måling

* Mål kørselstid med `clock_gettime()` eller `gettimeofday()`.
* Sammenlign versioner (tekst-sammenligning vs. bitmasker vs. multi-thread).

---

