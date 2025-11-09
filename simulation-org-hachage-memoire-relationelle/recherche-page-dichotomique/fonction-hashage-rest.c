#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> // for malloc, free

#define num_bloc 100
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

bloc memory[5]; // unchanged (still 5 blocs total)

// ---------------- rest of code unchanged ----------------

int hashCode(int key) {
   int n = key % num_bloc;
   return n;
}

bool verify_page_sature(int bloc_index, int page_index) {
    bool page_a_espace = false;
    int i = 0;
    while (i < taille_page && page_a_espace) {
        if (memory[bloc_index].pages[page_index][i].matricule == 0) {
            page_a_espace = true;
        }
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

int extend_bloc(int bloc_index, int taille_bloc) {
    int new_taille_bloc = taille_bloc;
    bool bloc_saturé = verify_if_bloc_sature(bloc_index, taille_bloc);
    if (bloc_saturé) {
        new_taille_bloc = taille_bloc + 1;
        memory[bloc_index].pages = realloc(memory[bloc_index].pages, new_taille_bloc * sizeof(page));
        memory[bloc_index].nb_pages = new_taille_bloc;
    }
    return new_taille_bloc;
}
int recherche_dans_page(int matricule, etudiant page[]) {
    int left = 0;
    int right = taille_page;
    int mat = -3;
    while (mat != matricule) {
        int middle = (left +right)*0.5;
        if (page[middle].matricule == matricule) {
            return middle;
            mat = matricule;
        } else if (page[middle].matricule > matricule) {
            right = middle;
        } else {
            left = middle;
        }
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
    int i = 0;
    while (i < taille_page) {
        if (page[i].matricule == NULL) {
            return i;
        }
    }
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
    if (verify_page_sature(bloc_index, page_index) == true) {
     
    if (verify_if_bloc_sature(bloc_index, page_index) == true) {
        extend_bloc(bloc_index, taille_bloc(bloc_index));

    }
}  
    int new_taille_bloc = taille_bloc(bloc_index);
    int i = 0;

    while (key > pages[page_index][i].matricule && i < taille_page) {
        i++;
    }
    for (int j = new_taille_bloc -1; j > page_index; j--) {
        for (int k = taille_page - 1; k >= 0; k--) {
            if (k == 0) {
                pages[j][k] = pages[j-1][taille_page -1];
                continue;
            }
            pages[j][k] = pages[j][k-1];
        }
    }
    for (int j = taille_page -1; j > i; j--) {
        pages[page_index][j] = pages[page_index][j-1];

}
    pages[page_index][i] = e;
}

void insertion(etudiant e) {
    int bloc_index = hashCode(e.matricule);
    bool space_in_page = false;
    int i = 0;
    while (i < taille_page && !space_in_page) {
    space_in_page = verify_page_sature(bloc_index, i);
    i++;
    }
    int taille_bloc_now = taille_bloc(bloc_index);
    decalage_page_pour_insertion(memory[bloc_index].pages, e.matricule, i-1, bloc_index, e);
}


int main() {
    printf("debugging function HashCode: \n");
    for (int i = 0; i < 20; i ++) {
       int bloc_pos = hashCode(i);
       printf("enregistrement num : %d, au bloc position : %d\n", i+1, bloc_pos);
       printf("-----------");
    }

    // 🔹 Initialize blocs dynamically
    for (int i = 0; i < 5; i++) {
        memory[i].nb_pages = 5; // initial number of pages per bloc
        memory[i].pages = malloc(memory[i].nb_pages * sizeof(page));
        if (!memory[i].pages) {
            printf("Erreur d'allocation pour bloc %d\n", i);
            return 1;
        }
    }

    printf("remplissage table de blocs: \n");
    for (int i = 0; i < num_bloc; i++) {
        bloc_table[i] = i;
    }

    printf("affichage table de blocs: \n");
    for (int i = 0; i < num_bloc; i++) {
        printf("%d\n", i);
    }

    // 🔹 Free blocs
    for (int i = 0; i < 5; i++) {
        free(memory[i].pages);
    }

    return 0;
}
