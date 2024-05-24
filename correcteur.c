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

uint8_t crcGeneration(uint8_t m){
    uint8_t crc = 0;

    for (int i = 0; i < 8; i++){
        if ((crc ^ m) & 0x80){
            crc = (crc << 1) ^ POLYNOME;
        }else{
            crc <<= 1;
        }
        m <<= 1;
    }

    return crc;
}


uint8_t crcVerif(uint16_t m) {
    uint8_t message = (m >> 8) & 0xFF;    // Extraire les 8 bits de message
    uint8_t error = m & 0xFF;             // Extraire les 8 bits de code d'erreur

    uint8_t new_crc = crcGeneration(message);
    if (new_crc == error) {
        return 0;
    } else {
        return new_crc;
    }
}


int crc_error_amount(uint16_t m){
    for (int i = 8; i < 16; ++i) {
        m = chg_nth_bit(i, m);

        if (crcVerif(m) == 0){
            return i-8;
        }

        m = chg_nth_bit(i, m);
    }

    return -1;
}


uint16_t concat(uint8_t m, uint8_t crc) {
    uint16_t message = m >> 8;
    message <<= 8;
    message |= crc;
    return message;
}

uint16_t encode_G(uint8_t m) {
    uint8_t crc = crcGeneration(m);
    return concat(m, crc);
}
