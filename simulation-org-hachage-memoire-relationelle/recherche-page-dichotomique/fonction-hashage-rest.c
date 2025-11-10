#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> // for malloc, free
#include <string.h>   // for strcpy
#include <assert.h>   // optional: for sanity checks

#define DEBUG_SHIFT 1
#if DEBUG_SHIFT
  #define TRACE(...) do { printf(__VA_ARGS__); putchar('\n'); fflush(stdout); } while(0)
#else
  #define TRACE(...) do {} while(0)
#endif

#define num_bloc 20
int bloc_table[num_bloc];

typedef struct etudiant {
    int matricule;
    char name[50];
    char surname[50];
} etudiant;
typedef struct posPage_insidePage {
    int num_page;
    int position_dans_page;
} posPage_insidePage;

#define taille_page 6
typedef etudiant page[taille_page];

// OLD: typedef page bloc[5];
// NEW: define bloc as a struct with dynamic allocation
typedef struct {
    page *pages;      // dynamically allocated array of pages
    int nb_pages;     // number of pages currently allocated
} bloc;

bloc memory[num_bloc]; // unchanged (still 5 blocs total)

// ---------------- rest of code unchanged ----------------

int hashCode(int key) {
   int n = key % num_bloc;
   return n;
}

bool verify_page_sature(int bloc_index, int page_index) {
    bool page_a_espace = false;
    int i = 0;
    while (i < taille_page && !page_a_espace) {          // <-- fix
        if (memory[bloc_index].pages[page_index][i].matricule == 0)
            page_a_espace = true;
        i++;
    }
    return page_a_espace;
}


bool verify_if_bloc_sature(int bloc_index, int taille_bloc) {
    int j = 0;
    bool page_a_espace = false;
    while (!page_a_espace && j < taille_bloc) {
        page_a_espace = verify_page_sature(bloc_index, j);
        j++;
    }
    return page_a_espace;
}

int taille_bloc(int bloc_index) {
    return memory[bloc_index].nb_pages;
}

int extend_bloc(int bloc_index, int taille_actuelle) {
    int new_taille = taille_actuelle;
    bool has_space = verify_if_bloc_sature(bloc_index, taille_actuelle);
    if (!has_space) {  // extend only when all pages are full
        new_taille = taille_actuelle + 1;
        page *new_pages = realloc(memory[bloc_index].pages, new_taille * sizeof(page));
        if (!new_pages) { perror("realloc"); exit(1); }
        memory[bloc_index].pages = new_pages;
        memory[bloc_index].nb_pages = new_taille;

        // zero-init the newly added page (index = taille_actuelle)
        for (int k = 0; k < taille_page; ++k) {
            memory[bloc_index].pages[taille_actuelle][k].matricule = 0;
            memory[bloc_index].pages[taille_actuelle][k].name[0] = '\0';
            memory[bloc_index].pages[taille_actuelle][k].surname[0] = '\0';
        }
        TRACE("[extend] bloc %d had no free slots across %d pages -> extended to %d pages and zero-initialized new page",
              bloc_index, taille_actuelle, new_taille);
    }
    return new_taille;
}

int recherche_dans_page(int matricule, etudiant page[]) {
    int left = 0, right = taille_page - 1;
    while (left <= right) {
        int middle = left + (right - left) / 2;
        int m = page[middle].matricule;

        if (m == 0) {           // treat empty slots as after filled data
            right = middle - 1; // or break if pages are tightly packed
            continue;
        }
        if (m == matricule) return middle;
        if (m > matricule) right = middle - 1; else left = middle + 1;
    }
    return -2;
}

posPage_insidePage recherche_dans_block(int block_index, int matricule) {
    posPage_insidePage p;
    int taille = taille_bloc(block_index);
    for (int i = 0; i < taille; i++) {
        int verif= recherche_dans_page(matricule, memory[block_index].pages[i]);
        if (verif != -2) {
            p.num_page = i;
            p.position_dans_page = verif;
            return p;
        }
    }
    p.num_page = -2;
    p.position_dans_page = -2;
    return p;

}
int pos_page_vide(etudiant page[]) {
    for (int i = 0; i < taille_page; ++i)           // simpler & safe
        if (page[i].matricule == 0) return i;
    return -1;
}

void decalage_page_pour_insertion(page pages[], int key, int page_index, int bloc_index, etudiant e) {
    if (page_index < 0 || page_index >= taille_bloc(bloc_index)) {
        printf("Erreur: index de page invalide %d pour le bloc %d\n", page_index, bloc_index);
        return;
    }
    if (key < 0) {
        printf("Erreur: clé invalide %d pour l'insertion\n", key);
        return;
    }

    int before_pages = taille_bloc(bloc_index);
    bool bloc_has_space = verify_if_bloc_sature(bloc_index, before_pages);
    bool page_has_space = verify_page_sature(bloc_index, page_index);
    TRACE("[plan] I am inserting matricule=%d into bloc=%d page=%d. Page has space? %s. Bloc has space somewhere? %s. Pages in bloc=%d",
          key, bloc_index, page_index, page_has_space ? "YES" : "NO", bloc_has_space ? "YES" : "NO", before_pages);

    if (!verify_if_bloc_sature(bloc_index, taille_bloc(bloc_index))) {
        if (!verify_page_sature(bloc_index, page_index)) {
            TRACE("[plan] Target page is full and bloc has no space elsewhere -> extend bloc before shifting");
            extend_bloc(bloc_index, taille_bloc(bloc_index));
        }
    }

    int new_taille_bloc = taille_bloc(bloc_index);

    // ASCENDING ORDER scan: stop at first empty slot OR first element >= key
    int i = 0;
    while (i < taille_page
        && pages[page_index][i].matricule != 0
        && key > pages[page_index][i].matricule) {
        TRACE("[scan] At bloc=%d page=%d index=%d: current=%d < key=%d -> keep scanning (ascending)",
              bloc_index, page_index, i, pages[page_index][i].matricule, key);
        i++;
    }
    TRACE("[decision] ASCENDING: I will open slot at i=%d in bloc=%d page=%d (reason: first empty or first >= key)",
          i, bloc_index, page_index);

    // Only shift tail pages if target page is actually full
    bool page_full = (pages[page_index][taille_page - 1].matricule != 0);
    if (page_full) {
        TRACE("[tail-shift] Target page is full, I must propagate to later pages");
        for (int j = new_taille_bloc - 1; j > page_index; j--) {
            TRACE("[tail-shift] shift page %d right (carry from page %d)", j, j-1);
            for (int k = taille_page - 1; k >= 0; k--) {
                if (k == 0) {
                    int src = pages[j - 1][taille_page - 1].matricule;
                    TRACE("  [move] pages[%d][%d] = pages[%d][%d] (src=%d)  // carry tail forward",
                          j, k, j-1, taille_page-1, src);
                    pages[j][k] = pages[j - 1][taille_page - 1];
                } else {
                    int src = pages[j][k - 1].matricule;
                    TRACE("  [move] pages[%d][%d] = pages[%d][%d] (src=%d)  // shift inside page",
                          j, k, j, k-1, src);
                    pages[j][k] = pages[j][k - 1];
                }
            }
        }
    } else {
        TRACE("[tail-shift] Skipped: target page has free space, no need to propagate to later pages");
    }

    TRACE("[in-page-shift] Shift inside bloc=%d page=%d from right down to i=%d", bloc_index, page_index, i);
    for (int j = taille_page - 1; j > i; j--) {
        int src = pages[page_index][j - 1].matricule;
        TRACE("  [move] pages[%d][%d] = pages[%d][%d] (src=%d)  // free index %d",
              page_index, j, page_index, j-1, src, i);
        pages[page_index][j] = pages[page_index][j - 1];
    }

    TRACE("[insert] Put matricule=%d at bloc=%d page=%d index=%d (ascending order preserved)",
          e.matricule, bloc_index, page_index, i);
    pages[page_index][i] = e;

    TRACE("[dump] State of bloc=%d page=%d after insertion:", bloc_index, page_index);
    for (int t = 0; t < taille_page; ++t) {
        TRACE("  pages[%d][%d].matricule=%d", page_index, t, pages[page_index][t].matricule);
    }
}
void insertion(etudiant e) {
    int bloc_index = hashCode(e.matricule);
    TRACE("[route] Student matricule=%d hashes to bloc=%d. I will find a page with space or extend.", e.matricule, bloc_index);

    int nbp = taille_bloc(bloc_index);
    int page_index = -1;
    for (int p = 0; p < nbp; ++p) {
        bool has_space = verify_page_sature(bloc_index, p);
        TRACE("[probe] Check bloc=%d page=%d for free slot -> %s", bloc_index, p, has_space ? "HAS SPACE" : "FULL");
        if (has_space) { page_index = p; break; }
    }
    if (page_index == -1) {
        int old = nbp;
        TRACE("[route] No page had space in bloc=%d. I extend the bloc from %d to %d pages and will use the new page index %d",
              bloc_index, old, old+1, old);
        extend_bloc(bloc_index, old); // adds one page
        page_index = old;             // use new page
    } else {
        TRACE("[route] I will insert into existing free page: bloc=%d page=%d", bloc_index, page_index);
    }
    decalage_page_pour_insertion(memory[bloc_index].pages, e.matricule, page_index, bloc_index, e);
}


// ---------- debug helpers ----------
static void zero_init_page(page p) {
    for (int i = 0; i < taille_page; i++) {
        p[i].matricule = 0;
        p[i].name[0] = '\0';
        p[i].surname[0] = '\0';
    }
}

static void zero_init_bloc(int b) {
    for (int i = 0; i < memory[b].nb_pages; i++) {
        zero_init_page(memory[b].pages[i]);
    }
}

static void print_page(const page p, int bloc_index, int page_index) {
    printf("\n[Bloc %d - Page %d]\n", bloc_index, page_index);
    printf(" idx | matricule | name            | surname\n");
    printf("-----+-----------+-----------------+-----------------\n");
    for (int i = 0; i < taille_page; i++) {
        printf(" %3d | %9d | %-15s | %-15s\n",
               i, p[i].matricule, p[i].name, p[i].surname);
    }
}

static void print_bloc(int b) {
    printf("\n========== BLOC %d (pages=%d) ==========\n", b, memory[b].nb_pages);
    for (int i = 0; i < memory[b].nb_pages; i++) {
        print_page(memory[b].pages[i], b, i);
    }
    printf("========================================\n");
}

static etudiant make_student(int mat, const char* name, const char* surname) {
    etudiant e;
    e.matricule = mat;
    strncpy(e.name, name, sizeof(e.name)-1);
    e.name[sizeof(e.name)-1] = '\0';
    strncpy(e.surname, surname, sizeof(e.surname)-1);
    e.surname[sizeof(e.surname)-1] = '\0';
    return e;
}

// simple wrapper to try searching and print result
static void try_search(int bloc_index, int matricule) {
    posPage_insidePage r = recherche_dans_block(bloc_index, matricule);
    if (r.num_page >= 0) {
        printf("Search %d -> found at bloc=%d page=%d pos=%d\n",
               matricule, bloc_index, r.num_page, r.position_dans_page);
    } else {
        printf("Search %d -> NOT FOUND in bloc %d\n", matricule, bloc_index);
    }
}

// ---------- MAIN ----------
int main(void) {
    // 0) Show hash spread for first 20 numbers
    printf("debugging function hashCode:\n");
    for (int i = 0; i < 20; i++) {
        int bloc_pos = hashCode(i);       // NOTE: requires num_bloc == 5
        printf("record #%d -> bloc %d\n", i+1, bloc_pos);
    }

    // 1) Allocate blocs
    printf("\nAllocating blocs...\n");
    for (int i = 0; i < num_bloc; i++) {
        memory[i].nb_pages = 2; // start small to test extend later
        memory[i].pages = (page*)malloc(memory[i].nb_pages * sizeof(page));
        if (!memory[i].pages) {
            printf("Allocation error for bloc %d\n", i);
            return 1;
        }
        zero_init_bloc(i); // set all matricules to 0 (empty)
    }

    // 2) Fill bloc table (if you keep it)
    for (int i = 0; i < num_bloc; i++) {
        bloc_table[i] = i;
    }

    // 3) Insert a handful of students (choose values that collide to test same bloc)
    //    With num_bloc == 5, numbers mod 5 decide bloc.
    //    We'll try to pack bloc 0 heavily to force shifts/extension.
    etudiant s1 = make_student(  5, "Alice",   "A");
    etudiant s2 = make_student( 10, "Bob",     "B");
    etudiant s3 = make_student( 15, "Charlie", "C");  // all three map to bloc 0
    etudiant s4 = make_student( 20, "Dora",    "D");  // bloc 0 again
    etudiant s5 = make_student(  1, "Eve",     "E");  // bloc 0 again
    etudiant s6 = make_student( 25, "Frank",   "F");  // bloc 0 again (should force page movement/extend)
    etudiant s7 = make_student(  27, "Gina",    "G");  // bloc 2
    etudiant s8 = make_student(  37, "Gina",    "G");  // bloc 2
    etudiant s9 = make_student(  74, "Gina",    "G");  // bloc 2
    etudiant s10 = make_student(  777, "Gina",    "G");  // bloc 2
    etudiant s11 = make_student(  76, "Gina",    "G"); 
    etudiant e1 = make_student(  7, "Gina",    "G");  // bloc 2
    etudiant e2 = make_student(  999, "Gina",    "G"); 

    printf("\nInserting into hash table (expect many into bloc 0):\n");
    insertion(s1);
    insertion(s2);
    insertion(s3);
    insertion(s4);
    insertion(s5);
    insertion(s6);
    insertion(s7);
    insertion(s8);
    insertion(s9);
    insertion(s10);
    insertion(s11);
    insertion(e1);
    insertion(e2);

    // 4) Print blocs to inspect layout
    for (int b = 0; b < num_bloc; b++) {
        print_bloc(b);
    }

    // 5) Try some searches
    printf("\nSearch tests:\n");
    int b0 = hashCode(5);
    try_search(hashCode(5), 5);
    try_search(b0, 25);
    try_search(hashCode(7), 7);
    try_search(hashCode(999), 999);

    // 6) Optional: test verify_page_sature & extend_bloc explicitly on bloc 0
    printf("\nManual saturation test on bloc 0:\n");
    int b = 0;
    for (int p = 0; p < memory[b].nb_pages; p++) {
        bool has_space = verify_page_sature(b, p);
        printf("Bloc %d, page %d has space? %s\n", b, p, has_space ? "YES" : "NO");
    }
    int old_pages = taille_bloc(b);
    int new_pages = extend_bloc(b, old_pages);
    printf("extend_bloc(0, %d) -> %d pages now\n", old_pages, new_pages);

    // 7) Final state
    for (int bb = 0; bb < 5; bb++) {
        print_bloc(bb);
    }

    // 8) Free memory
    for (int i = 0; i < num_bloc; i++) {
        free(memory[i].pages);
        memory[i].pages = NULL;
        memory[i].nb_pages = 0;
    }
    printf("\nDone.\n");
    return 0;
}