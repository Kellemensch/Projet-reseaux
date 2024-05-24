#include "correcteur.h"

uint16_t chg_nth_bit(int n, uint16_t m) {
    uint16_t mask = 1 << n;  // Créer un masque pour le n-ième bit (en partant de 0)
    return m ^ mask;  // Appliquer le masque avec l'opérateur "ou exclusif" binaire pour inverser le n-ième bit
}


uint16_t set_nth_bit(int n, uint16_t m) {
    uint16_t mask = 1 << n;  // Créer un masque pour le n-ième bit (en partant de 0)
    return m | mask;  // Appliquer le masque avec l'opérateur "ou" binaire pour mettre le n-ième bit à 1
}


uint8_t get_nth_bit(int n, uint8_t m) {
    uint8_t mask = 1 << n;  // Créer un masque pour le n-ième bit (en partant de 0)
    return (m & mask) >> n;  // Appliquer le masque avec l'opérateur "et" binaire pour récupérer le n-ième bit
}


void print_binary_8bit(uint8_t value) {
    for (int i = 7; i >= 0; i--) {  // Parcours les bits du poids fort au poids faible
        if ((value >> i) & 1) {  // Si le i-ème bit est 1
            printf("1");
        } else {  // Si le i-ème bit est 0
            printf("0");
        }
    }
}


void print_binary_16bit(uint16_t value) {
    for (int i = 15; i >= 0; i--) {  // Parcours les bits du poids fort au poids faible
        if ((value >> i) & 1) {  // Si le i-ème bit est 1
            printf("1");
        } else {  // Si le i-ème bit est 0
            printf("0");
        }
    }
}


void print_word(int k, uint16_t value) {
    for (int i = 15; i >= 16 - k; i--) {  // Parcours les bits du poids fort au poids faible
        if ((value >> i) & 1) {  // Si le i-ème bit est 1
            printf("1");
        } else {  // Si le i-ème bit est 0
            printf("0");
        }
    }
}


uint16_t encode_G(uint16_t m){
    uint16_t data = m & 0xFF00; // Garder les 8 bits de données
    uint16_t parity_bits = 0;

    // Pour chaque bit de donnée
    for (int i = 0; i < 8; i++) {
        if (get_nth_bit(1, data)) // Si le premier bit est 1 faire la division (XOR)
            data = data ^ POLYNOME;

        data = data << 1;
    }

    parity_bits = data >> 8; // 8bits les plus significatifs

    return (m & 0xFF00) | parity_bits;
}


int cardinal_bit(uint16_t m) {
    int cpt = 0;
    for (int i = 16; i > 0; i--) {  // Parcours les bits du poids fort au poids faible
        if (get_nth_bit(i, m)) {  // Si le i-ème bit est 1
            cpt++;
        }
    }
    return cpt;
}

int min_hamming_distance(uint16_t generator) {
    int min_distance = 16; // Initialiser à une valeur maximale

    // Examiner tous les mots de code possibles
    for (uint16_t i = 0; i < (1 << 8); ++i) {
        uint16_t encoded_word = encode_G(i >> 8); // Encoder les mots de 8 bits avec padding

        for (uint16_t j = i + 1; j < (1 << 8); ++j) {
            uint16_t encoded_word_2 = encode_G(j >> 8);
            uint8_t distance = cardinal_bit(encoded_word ^ encoded_word_2);

            if (distance < min_distance) {
                min_distance = distance;
            }
        }
    }

    return min_distance;
}

