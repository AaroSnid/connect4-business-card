#ifndef BOARD_H_
#define BOARD_H_

#include <stdint.h>
#if defined(__cplusplus)
#include <climits>
#else
#include <limits.h>
#include <stdbool.h>
#endif

#define BOARD_MASK 0xFC07E0FC07E03FULL

unsigned int MINIMAX_MAX_DEPTH = 5;           // In current implementation this is user selectable
#define TRANSPOSITION_TABLE_SIZE 0 // Prime number should be used to reduce the chance of collisions

#define HEURISTIC_3_LINE_WEIGHT 15
#define HEURISTIC_2_LINE_WEIGHT 4
#define HEURISTIC_CENTER_PIECE_WEIGHT 4

#if TRANSPOSITION_TABLE_SIZE != 0
enum TranspositionBound {
    TT_EXACT_BOUND,
    TT_LOWER_BOUND,
    TT_UPPER_BOUND
};

struct TranspositionEntry {
    uint64_t key;
    int score;
    uint8_t depth;
    uint8_t bound;
    bool valid;
};

static TranspositionEntry transposition_table[TRANSPOSITION_TABLE_SIZE] = {};
#endif

/*
Bit board:

.  .  .  .  .  .  .
5 12 19 26 33 40 47
4 11 18 25 32 39 46
3 10 17 24 31 38 45
2  9 16 23 30 37 44
1  8 15 22 29 36 43
0  7 14 21 28 35 42

uint64_t = 0 0 ...  0 47 46 45 44 43 42  0 40 39 38 37 36 35  0 33 32 31 30 29 28  0 26 25 24 23 22 21  0 19 18 17 16 15 14  0 12 11 10  9  8  7  0  5  4  3  2  1  0
pos % 7 gives row number
directions[4] = {1, 6, 7, 8};

*/

/* 
This is the code used to generate the 4 search masks

HeuristicDirectionMasks build_heuristic_direction_masks(){
    HeuristicDirectionMasks masks = {0, 0, 0, 0};
    for (int col = 0; col < 7; col++){
        for (int row = 0; row < 6; row++){
            const uint64_t bit = 1ULL << (7 * col + row);
            if (col <= 3) masks.horizontal |= bit;       // step 7
            if (row <= 2) masks.vertical |= bit;         // step 1
            if (col <= 3 && row <= 2) masks.diagonal_up |= bit;   // step 8
            if (col <= 3 && row >= 3) masks.diagonal_down |= bit; // step 6
        }
    }
    return masks;
}

struct HeuristicDirectionMasks {
    uint64_t horizontal;
    uint64_t vertical;
    uint64_t diagonal_up;
    uint64_t diagonal_down;
};

*/

#if TRANSPOSITION_TABLE_SIZE != 0
inline uint64_t compute_position_key(uint64_t game_board, uint64_t player_moves){
    // Mix both bitboards to uniquely represent side-to-move state.
    uint64_t key = game_board ^ (player_moves * 0x9E3779B97F4A7C15ULL);
    key ^= (key >> 33);
    key *= 0xFF51AFD7ED558CCDULL;
    key ^= (key >> 33);
    return key;
}

inline uint32_t transposition_index(uint64_t key){
    return static_cast<uint32_t>(key % TRANSPOSITION_TABLE_SIZE);
}

void clear_transposition_table(){
    for (int i = 0; i < TRANSPOSITION_TABLE_SIZE; i++){
        transposition_table[i].valid = false;
    }
}
#endif

/* This function will find any 4 adjacent pieces on the given board */
bool is_won(uint64_t player_moves){
    // Horizontal
    uint64_t two_shift = player_moves & (player_moves << 7);
    if (two_shift & (two_shift << 14)) return true;

    // Diagonal
    two_shift  = player_moves & (player_moves << 8);
    if (two_shift & (two_shift << 16)) return true;

    // Diagonal
    two_shift  = player_moves & (player_moves << 6);
    if (two_shift & (two_shift << 12)) return true;

    // Vertical
    two_shift = player_moves & (player_moves << 1);
    if (two_shift & (two_shift << 2)) return true;

    return false;
}

/* Check if all spaces in the bitboard are filled */
bool is_full(uint64_t game_board){
    return game_board == BOARD_MASK;
}

/* Returns 1 if the column is at capacity, 0 otherwise */
static inline bool is_playable(uint64_t game_board, uint8_t column){
    // Maps 0:5, 1:12, 2:19, etc
    return !(game_board & (1ULL << (7*column + 5)));
}

/* Finds the lowest open space in the given column */
int find_open_space(uint64_t game_board, uint8_t column){
    if (is_playable(game_board, column)){

        // Mask the bits of the desired column
        uint8_t masked_column = (game_board >> (7 * column)) & 0x7FULL;

        // Since we can guarantee the column is filled from the right, adding 1 will locate the first nonzero bit
        uint8_t first_empty_bit = masked_column + 1;

        // Count the number of trailing zeros in the column, and convert back into a bit index
        return __builtin_ctzll(first_empty_bit) + (7 * column);
    }
    // If the column is full, return -1
    return -1;
}

/* Plays the move in the given column. Returns the bit board position inserted at, or -1 for error */
int play_move(uint64_t *game_board, unsigned char column){
    int bit_index = find_open_space(*game_board, column);
    if (bit_index >= 0) *game_board |= (1ULL << bit_index);
    return bit_index;
}

// Checks if the move would retult in a win without modifying the given board
bool is_winning_move(uint64_t game_board, uint64_t player_moves, unsigned char column){
    uint64_t temp_board = game_board;
    int bit_index = play_move(&temp_board, column);
    if (bit_index < 0) return false;
    return is_won(player_moves | (1ULL << bit_index));
}

int evaluate_direction(uint64_t own_pieces, uint64_t opponent_pieces, uint64_t start_mask, int shift){
    const uint64_t b0 = own_pieces & start_mask;
    const uint64_t b1 = (own_pieces >> shift) & start_mask;
    const uint64_t b2 = (own_pieces >> (2 * shift)) & start_mask;
    const uint64_t b3 = (own_pieces >> (3 * shift)) & start_mask;

    const uint64_t o0 = opponent_pieces & start_mask;
    const uint64_t o1 = (opponent_pieces >> shift) & start_mask;
    const uint64_t o2 = (opponent_pieces >> (2 * shift)) & start_mask;
    const uint64_t o3 = (opponent_pieces >> (3 * shift)) & start_mask;
    const uint64_t blocked = o0 | o1 | o2 | o3;

    const uint64_t own_pieces3 =
        ((b0 & b1 & b2 & ~b3) |
         (b0 & b1 & ~b2 & b3) |
         (b0 & ~b1 & b2 & b3) |
         (~b0 & b1 & b2 & b3)) & ~blocked;
    const uint64_t own_pieces2 =
        ((b0 & b1 & ~b2 & ~b3) |
         (b0 & ~b1 & b2 & ~b3) |
         (b0 & ~b1 & ~b2 & b3) |
         (~b0 & b1 & b2 & ~b3) |
         (~b0 & b1 & ~b2 & b3) |
         (~b0 & ~b1 & b2 & b3)) & ~blocked;

    return __builtin_popcountll(own_pieces3) * HEURISTIC_3_LINE_WEIGHT
         + __builtin_popcountll(own_pieces2) * HEURISTIC_2_LINE_WEIGHT;
}

int board_heuristic(uint64_t game_board, uint64_t player_moves){
    const uint64_t opponent_moves = game_board ^ player_moves;
    const uint64_t mask_horizontal = 0x7EFDFBFULL;
    const uint64_t mask_vertical = 0x1C3870E1C387ULL;
    const uint64_t mask_diagonal_up = 0xE1C387ULL;
    const uint64_t mask_diagonal_down = 0x70E1C38ULL;
    int score = 0;

    // Center column control (column 3) has high combinational value.
    const uint64_t center_mask = 0x7E00000ULL;
    score += (__builtin_popcountll(player_moves & center_mask)
             - __builtin_popcountll(opponent_moves & center_mask)) * HEURISTIC_CENTER_PIECE_WEIGHT;

    score += evaluate_direction(player_moves, opponent_moves, mask_horizontal, 7);
    score += evaluate_direction(player_moves, opponent_moves, mask_vertical, 1);
    score += evaluate_direction(player_moves, opponent_moves, mask_diagonal_up, 8);
    score += evaluate_direction(player_moves, opponent_moves, mask_diagonal_down, 6);

    score -= evaluate_direction(opponent_moves, player_moves, mask_horizontal, 7);
    score -= evaluate_direction(opponent_moves, player_moves, mask_vertical, 1);
    score -= evaluate_direction(opponent_moves, player_moves, mask_diagonal_up, 8);
    score -= evaluate_direction(opponent_moves, player_moves, mask_diagonal_down, 6);

    return score;
}

int minimax(uint64_t game_board, uint64_t player_moves, uint8_t depth, int alpha, int beta){
    const uint64_t opponent_moves = game_board ^ player_moves;

    if (is_won(player_moves)) return 100000 - depth;
    if (is_won(opponent_moves)) return depth - 100000;
    if (is_full(game_board)) return 0;
    if (depth == MINIMAX_MAX_DEPTH) return board_heuristic(game_board, player_moves);

#if TRANSPOSITION_TABLE_SIZE != 0
    const int original_alpha = alpha;
    const uint8_t remaining_depth = MINIMAX_MAX_DEPTH - depth;

    const uint64_t position_key = compute_position_key(game_board, player_moves);
    TranspositionEntry &entry = transposition_table[transposition_index(position_key)];

    if (entry.valid && entry.key == position_key && entry.depth >= remaining_depth){
        if (entry.bound == TT_EXACT_BOUND) return entry.score;
        if (entry.bound == TT_LOWER_BOUND && entry.score > alpha) alpha = entry.score;
        else if (entry.bound == TT_UPPER_BOUND && entry.score < beta) beta = entry.score;
        if (alpha >= beta) return entry.score;
    }
#endif

    // Middle column out ordering improves alpha-beta pruning effectiveness.
    const unsigned char ordered_columns[7] = {3, 2, 4, 1, 5, 0, 6};

    int best_score = INT_MIN;
    for (int i = 0; i < 7; i++){
        unsigned char col = ordered_columns[i];
        int bit_index = find_open_space(game_board, col);
        if (bit_index < 0) continue;

        uint64_t next_game_board = game_board | (1ULL << bit_index);
        uint64_t next_player_moves = opponent_moves;

        // Negamax: evaluate child from opponent's perspective, then negate.
        int score = -minimax(next_game_board, next_player_moves, depth + 1, -beta, -alpha);
        if (score > best_score) best_score = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }

    int final_score = (best_score == INT_MIN) ? 0 : best_score;

#if TRANSPOSITION_TABLE_SIZE != 0
    uint8_t bound = TT_EXACT_BOUND;
    if (final_score <= original_alpha) bound = TT_UPPER_BOUND;
    else if (final_score >= beta) bound = TT_LOWER_BOUND;

    if (!entry.valid || remaining_depth >= entry.depth){
        entry.key = position_key;
        entry.score = final_score;
        entry.depth = remaining_depth;
        entry.bound = bound;
        entry.valid = true;
    }
#endif

    return final_score;
}

int choose_best_ai_move(uint64_t game_board, uint64_t p1_moves) {
    const unsigned char ordered_columns[7] = {3, 2, 4, 1, 5, 0, 6};
    const uint64_t ai_moves = game_board ^ p1_moves;
    int best_column = -1;
    int best_score = INT_MIN;

    for (int i = 0; i < 7; i++) {
        unsigned char col = ordered_columns[i];
        int bit_index = find_open_space(game_board, col);
        if (bit_index < 0) continue;

        uint64_t next_game_board = game_board | (1ULL << bit_index);
        uint64_t next_p1_moves = p1_moves;

        // Immediate wins are always preferred.
        if (is_won(ai_moves | (1ULL << bit_index))) return col;

        int score = -minimax(next_game_board, next_p1_moves, 1, -1000000, 1000000);
        if (score > best_score) {
            best_score = score;
            best_column = col;
        }
    }

    return best_column;
}

uint64_t isolate_winning_moves(uint64_t game_board) {
    uint8_t directions[4] = {1U, 6U, 7U, 8U};
    
    for (int i = 0; i < 4; ++i) {
        uint64_t two_shift = game_board & (game_board << directions[i]);
        two_shift = two_shift & (two_shift << 2*directions[i]);

        if (two_shift != 0U) {
            uint64_t line_bits = two_shift | two_shift >> directions[i];
            line_bits = line_bits | line_bits >> 2*directions[i];

            /* This function intentionally picks a single direction, rather
             * than all bits (if 2+ lines have been formed) */
            return line_bits;
        }
    }

    return 0;
}

#endif /* BOARD_H_ */