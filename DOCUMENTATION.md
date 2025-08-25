# Five Word Five Letters - C Implementation Dokumentation

## Projektoversigt

Dette projekt er designet til at løse "Five Word Five Letters" puslespillet. Målet er at finde alle mulige kombinationer af fem fem-bogstavs ord, hvor intet bogstav gentages på tværs af alle ord (hvert bogstav bruges præcis én gang).

## Tekniske Specifikationer

### Kernekrav
- Find alle kombinationer af 5 ord hvor:
  - Hvert ord skal være præcis 5 bogstaver langt
  - Intet bogstav kan gentages på tværs af de 5 ord
  - Hvert bogstav kan kun optræde én gang i alt
- Fokus på maksimal ydelse og effektiv hukommelsesudnyttelse

### Projektstruktur

```
modular/
├── bin/                 # Kompileret executable
│   └── word_combo_finder.exe
├── include/            # Header files
│   ├── file_io.h      # File operationer
│   ├── memory_utils.h # Memory management
│   ├── threading.h    # Threading utilities
│   ├── word_types.h   # Centrale data structures
│   └── word_utils.h   # Word processing
├── src/               # Source files
│   ├── file_io.c
│   ├── globals.c
│   ├── main.c
│   ├── memory_utils.c
│   ├── threading.c
│   └── word_utils.c
└── makefile           # Build konfiguration
```

## Kernekomponenter

### 1. Data Structures
```c
typedef struct {
    char word[WORDLEN + 1];  // Det faktiske ord (null-terminated)
    unsigned int mask;       // Bit mask for bogstav
} Word;

typedef struct {
    int start, end, thread_id;  // Thread arbejdsdeling parametre
} ThreadArg;
```

### 2. Nøglekonstanter
```c
#define MAX_WORDS 400000        // Maksimalt antal ord at behandle
#define WORDLEN 5              // Længde af hvert ord
#define COMBO_LEN 5            // Antal ord i kombination
#define MAX_MEMORY_PERCENTAGE 0.75  // Hukommelse brugsgrænse
#define MIN_MEMORY_MB 100      // Minimum hukommelse krav
```

## Implementeringsdetaljer

### Kernealgoritme
Programmet bruger flere teknikker til effektivt at finde gyldige ordkombinationer:

1. **Bit Masking System**
   - Hvert ord konverteres til en 32-bit mask hvor hver bit repræsenterer et bogstav
   - Programmet bruger `word_to_mask` funktionen til at konvertere ord:
   ```c
   unsigned int word_to_mask(const char* w) {
       unsigned int mask = 0;
       for(int i = 0; i < WORDLEN; i++) {
           char c = (char)(w[i] | 32);
           if(c < 'a' || c > 'z')
               return 0;
           int bit = LETTER_FREQ_POS[c - 'a'];
           mask |= 1u << bit;
       }
       return mask;
   }
   ```
   - Bogstavpositioner remappes baseret på frekvens for optimering
   - Duplikatbogstaver i et ord opdages automatisk under mask oprettelse

2. **Kombination Finding Algoritme**
   - Bruger nested loops med bit mask operationer for effektivitet
   - Udfører tidlig beskæring af ugyldige kombinationer ved brug af bit operationer
   - Kontrollerer kombinationer ved brug af bitwise AND operationer for at opdage bogstav overlap
   - Tæller bits ved brug af optimerede hardware instruktioner når tilgængelige:
   ```c
   int bit_count_u32(unsigned int x) {
       #if defined(__clang__) || defined(__GNUC__)
           return __builtin_popcount(x);
       #elif defined(_MSC_VER)
           return __popcnt(x);
       #else
           // Fallback bit counting implementation
       #endif
   }
   ```

### Threading Implementation
1. **Thread Management**
   - Opdager automatisk optimal thread antal ved brug af system CPU information
   - Hver thread behandler et distinkt område af ordlisten
   - Bruger følgende thread arbejdsdeling struktur:
   ```c
   typedef struct {
       int start, end, thread_id;
   } ThreadArg;
   ```

2. **Parallel Processing**
   - Implementerer en worker funktion som behandler ordkombinationer:
   ```c
   void* worker(void* arg) {
       ThreadArg* t = (ThreadArg*)arg;
       for(int i = t->start; i < t->end; i++) {
           // Behandl kombinationer startende med word[i]
       }
   }
   ```
   - Bruger atomic operationer for thread-safe tælling af gyldige kombinationer
   - Hver thread opretholder en lokal tæller for at minimere atomic operation overhead

### Memory Management
1. **Dynamic Allocation**
   - Beregner sikre hukommelsesgrænser baseret på systemressourcer
   - Implementerer hukommelse bounds kontrol
   - Bruger et struktureret Word array for effektiv cache udnyttelse:
   ```c
   typedef struct {
       char word[WORDLEN + 1];
       unsigned int mask;
   } Word;
   ```

2. **Memory Optimering**
   - Pre-allokerer plads til maksimalt forventet ordantal
   - Organiserer data structures for cache-venlige adgangsmønstre
   - Implementerer hukommelsesgrænse beregninger for at forhindre systemoverbelastning

### Word Processing og Validering
1. **Word Validering**
   - Kontrollerer for præcis ordlængde (5 bogstaver)
   - Validerer tegnsæt (A-Z eller a-z kun)
   - Opdager og eliminerer duplikatbogstaver inden for ord
   - Udfører case-insensitive behandling

2. **Optimeringsteknikker**
   - Sorterer ord efter bit count for bedre beskæring effektivitet
   - Bruger bogstavfrekvens information til optimerede bit tildeling
   - Implementerer tidlige exit betingelser i kombinationskontrol

### File I/O System
1. **Word Loading Process**
   ```c
   int load_words_from_file(const char* inputPath, size_t maxWords) {
       FILE* f = fopen(inputPath, "r");
       while(wordCount < (int)maxWords && fscanf(f, "%63s", buf) == 1) {
           if((int)strlen(buf) != WORDLEN)
               continue;
           
           unsigned int mask = word_to_mask(buf);
           if(mask && bit_count_u32(mask) == WORDLEN) {
               // Gem gyldigt ord og dets mask
               memcpy(words[wordCount].word, buf, WORDLEN);
               words[wordCount].mask = mask;
               wordCount++;
           }
       }
   }
   ```
   - Implementerer effektiv bufferet fil læsning
   - Udfører validering under indlæsning for at spare hukommelse
   - Opretholder sporing af gyldige vs totale ord læst
   - Sikrer korrekte hukommelsesgrænser under indlæsning

2. **Input Processing**
   - Filtrerer ord der ikke opfylder længdekrav
   - Validerer ordindhold under indlæsningsprocessen
   - Opretter bit masks samtidig med indlæsning
   - Opretholder statistik om indlæsningsprocessen

### Avanceret Memory Management
1. **System Memory Analyse**
   ```c
   size_t getAvailableSystemMemoryMB(void) {
       MEMORYSTATUSEX status;
       status.dwLength = sizeof(status);
       GlobalMemoryStatusEx(&status);
       return (size_t)(status.ullAvailPhys / (1024 * 1024));
   }
   ```
   - Forespørger systemet for tilgængelig fysisk hukommelse
   - Beregner sikre hukommelsesgrænser baseret på tilgængelige ressourcer
   - Implementerer sikkerhedsforanstaltninger mod hukommelsesudmattelse

2. **Dynamic Resource Allocation**
   ```c
   size_t calculateSafeMemoryLimit(void) {
       size_t availableMB = getAvailableSystemMemoryMB();
       size_t safeLimitMB = (size_t)(availableMB * MAX_MEMORY_PERCENTAGE);
       return (safeLimitMB < MIN_MEMORY_MB) ? MIN_MEMORY_MB : safeLimitMB;
   }
   ```
   - Bruger procent-baserede hukommelse allokeringsgrænser
   - Sikrer minimum påkrævet hukommelse tilgængelighed
   - Beregner ordkapacitet baseret på tilgængelig hukommelse:
   ```c
   size_t calculateMaxWords(size_t memoryLimitMB) {
       size_t totalBytes = memoryLimitMB * 1024 * 1024;
       return totalBytes / sizeof(Word);
   }
   ```

3. **Memory Overvågning**
   - Leverer detaljerede hukommelsesstatistikker under udførelse
   - Overvåger hukommelsesforbrug gennem hele behandlingen
   - Implementerer hukommelse-bevidste data structures
   - Rapporterer hukommelsesstatistik til bruger:
   ```c
   void printMemoryInfo(void) {
       printf("System Memory Info:\n");
       printf("Available Memory: %zu MB\n", getAvailableSystemMemoryMB());
       printf("Using Memory Limit: %zu MB\n", memoryLimit);
       printf("Max Words Capacity: %zu\n", maxWords);
   }
   ```

### Performance Optimering
1. **Cache Optimering**
   - Strukturerer data for optimal cache line brug
   - Ordner operationer for at maksimere cache hits
   - Minimerer cache misses i kritiske loops

2. **CPU Optimering**
   - Bruger CPU-specifikke instruktioner når tilgængelige
   - Implementerer branch prediction venlige loops
   - Organiserer data for SIMD-venlig behandling

3. **Memory Access Patterns**
   - Sekventielle adgangsmønstre for bedre prefetching
   - Minimerer pointer chasing i kritiske paths
   - Batches lignende operationer for bedre pipelining

## Brug

### Command Line Argumenter
```
word_combo_finder [-i input_file] [-t thread_count]
```
- `-i`: Specificer input file path (standard: "5Words5Letters/lists/unique_words.txt")
- `-t`: Specificer antal threads at bruge

### Input Fil Krav
- Et ord per linje
- Ord skal være præcis 5 bogstaver lange
- Kun store bogstaver A-Z accepteres
- Ingen duplikatord tilladt

## Build Instruktioner

1. Sørg for at have GCC installeret
2. Naviger til projektmappen
3. Kør `make` for at bygge projektet
4. Den executable vil blive oprettet i `bin` mappen

## Performance Overvejelser

1. **Memory Optimering**
   - Bruger bit masks for effektiv bogstavsporing
   - Implementerer hukommelsesgrænser for systemsikkerhed
   - Optimerer data structures for cache effektivitet

2. **Threading**
   - Parallel processing for hurtigere beregning
   - Atomic operationer for thread sikkerhed
   - Effektiv arbejdsdeling mellem threads

3. **Algoritme Effektivitet**
   - Bogstavfrekvens-baserede optimeringer
   - Effektiv validering ved brug af bit operationer
   - Optimerede data structures for hurtig adgang

## Fejlhåndtering

Programmet inkluderer omfattende fejlhåndtering for:
- Memory allocation fejl
- File I/O fejl
- Ugyldig input validering
- Thread oprettelse og management
- System ressource begrænsninger

## Test og Verifikation

For at verificere program funktionalitet:
1. Kør med små test filer først
2. Verificer output kombinationer er gyldige
3. Kontroller hukommelsesforbrug under forskellige betingelser
4. Test med maksimal ordliste størrelse
5. Verificer thread sikkerhed med forskellige thread antal