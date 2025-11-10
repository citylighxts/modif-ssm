#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#define VERBOSE 1
#define VERBOSE_RECTANGLE_SEARCH 1

// Nilai Optimal per data N01.N35 

const int optimal_solution[36] = {
    0,
    5600, 28, 1102, 1580, 410, 7750, 1160, -1, 111, // N01.N09
    910, 2460, 425, 4525, 920, 156, 1510, 743, 1650, // N10.N18
    -1, -1, 4205, 12075, 23, 3460, 809, 490, 59356, // N19.N27
    2090, 3857, 465, 1480, 1780, 1250, 510, 280 // N28.N35        
};

/* Hitung total biaya */
static long long total_biaya(int m, int n, int cost[m][n], int alloc[m][n]) {
    long long tot = 0;
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            tot += (long long)cost[i][j] * (long long)alloc[i][j];
    return tot;
}

/* Helper printing utilities */
static void print_alloc_matrix_int(int m, int n, int alloc[m][n]) {
    if (!VERBOSE) return;
    printf("\n+---------------------- ALLOCATION MATRIX (%dx%d) ----------------------+\n", m, n);
    // header kolom (1-based)
    printf("        ");
    for (int j = 0; j < n; j++) printf("   c%02d  ", j+1);
    printf("\n--------------------------------------------------------------------------\n");
    for (int i = 0; i < m; i++) {
        printf("r%02d | ", i+1);
        for (int j = 0; j < n; j++) printf("%7d ", alloc[i][j]);
        printf("\n");
    }
    printf("+------------------------------------------------------------------------+\n");
}

static void print_cost_matrix(int m, int n, int cost[m][n], int demand[n], int supply[m]) {
    if (!VERBOSE) return;
    printf("\n+------------------------- COST MATRIX (%dx%d) -------------------------+\n", m, n);
    printf("        ");
    for (int j = 0; j < n; j++) printf("  c%02d  ", j+1);
    printf("\n--------------------------------------------------------------------------\n");
    for (int i = 0; i < m; i++) {
        printf("r%02d | ", i+1);
        for (int j = 0; j < n; j++) printf("%6d ", cost[i][j]);
        printf("| %6d", supply[i]);
        printf("\n");
    }
    printf("+------------------------------------------------------------------------+\n");
    // print demand
    printf("     ");
    for (int j = 0; j < n; j++) {
        printf("  %5d", demand[j]);
    }
    printf("\n");
}

static void print_array_ll(const char *name, int len, long long arr[]) {
    if (!VERBOSE) return;
    printf("%s: [", name);
    for (int i = 0; i < len; i++) printf("%lld%s", arr[i], (i == len-1) ? "" : ", ");
    printf("]\n");
}

static void print_array_int(const char *name, int len, int arr[]) {
    if (!VERBOSE) return;
    printf("%s: [", name);
    for (int i = 0; i < len; i++) printf("%d%s", arr[i], (i == len-1) ? "" : ", ");
    printf("]\n");
}

static void print_supply_status(int m, int supply[], int total_alloc_baris[]) {
    if (!VERBOSE) return;
    printf("\nSupply status (supply / alokasi / sisa):\n");
    printf("------------------------------------------------\n");
    printf("  Row | Supply |  Alokasi  | Sisa\n");
    printf("------+--------+-----------+----------\n");
    for (int i = 0; i < m; i++) {
        int remain = supply[i] - total_alloc_baris[i];
        printf(" r%02d  | %6d | %9d | %8d\n", i+1, supply[i], total_alloc_baris[i], remain);
    }
    printf("------------------------------------------------\n");
}

// Fase optimisasi: perbaikan rectangular (4-sudut) yang di-looping terus-menerus

static void improve_with_rectangles(int m, int n, int cost[m][n], int alloc[m][n]) {
    int iter = 0;

    if (VERBOSE) {
        printf("\n=== RECTANGLE IMPROVEMENT PHASE: START ===\n");
        print_alloc_matrix_int(m, n, alloc);
    }
    
    // Loop utama yang akan terus berjalan selama perbaikan masih ditemukan
    while (true) {
        int best_r1 = -1, best_c1 = -1, best_r2 = -1, best_c2 = -1;
        int best_delta = 0;
        bool is_type2_move = false; // Flag untuk menandai tipe perbaikan
        
        int rect_counter = 0; // Menghitung jumlah rectangle yang dicek per iterasi
        if (VERBOSE_RECTANGLE_SEARCH) {
            printf("\n[Pencarian Iterasi %d] Memeriksa semua kemungkinan rectangle...\n", iter + 1);
        }

        // Cari perbaikan terbaik di seluruh tabel
        for (int r1 = 0; r1 < m; r1++) {
            for (int c1 = 0; c1 < n; c1++) {
                for (int r2 = r1 + 1; r2 < m; r2++) {
                    for (int c2 = c1 + 1; c2 < n; c2++) {
                        
                        rect_counter++;
                        if (VERBOSE_RECTANGLE_SEARCH) {
                            printf("---\n[rect-%03d] Cek (r%d,c%d)-(r%d,c%d)\n", 
                                   rect_counter, r1+1, c1+1, r2+1, c2+1);
                        }

                        // Tipe 1: Pindahkan alokasi DARI (r1,c1) & (r2,c2)
                        // Donors: (r1,c1) , (r2,c2)
                        // Penerima: (r1,c2) , (r2,c1)
                        int delta1 = (cost[r1][c2] + cost[r2][c1]) - (cost[r1][c1] + cost[r2][c2]);
                        
                        // Cek Alokasi Donor Tipe 1
                        if (alloc[r1][c1] > 0 && alloc[r2][c2] > 0) {
                            if (VERBOSE_RECTANGLE_SEARCH) {
                                printf("    [T1] Cek donor (r%d,c%d)=%d, (r%d,c%d)=%d. OK.\n",
                                       r1+1, c1+1, alloc[r1][c1], r2+1, c2+1, alloc[r2][c2]);
                                printf("    [T1] Delta = (cost[r%d][c%d] + cost[r%d][c%d]) - (cost[r%d][c%d] + cost[r%d][c%d])\n",
                                       r1+1, c2+1, r2+1, c1+1, r1+1, c1+1, r2+1, c2+1);
                                printf("    [T1] Delta = (%d + %d) - (%d + %d) = %d\n",
                                       cost[r1][c2], cost[r2][c1], cost[r1][c1], cost[r2][c2], delta1);
                            }
                            if (delta1 < best_delta) {
                                best_delta = delta1;
                                best_r1 = r1; best_c1 = c1;
                                best_r2 = r2; best_c2 = c2;
                                is_type2_move = false;
                                if (VERBOSE) {
                                    printf("    [T1] => Perbaikan T1 ditemukan! Delta baru = %d.\n", delta1);
                                }
                            } else if (VERBOSE_RECTANGLE_SEARCH) {
                                printf("    [T1] Skip: delta %d >= best_delta %d.\n", delta1, best_delta);
                            }
                        } else if (VERBOSE_RECTANGLE_SEARCH) {
                            printf("    [T1] Skip: donor (r%d,c%d)=%d atau (r%d,c%d)=%d adalah 0.\n",
                                   r1+1, c1+1, alloc[r1][c1], r2+1, c2+1, alloc[r2][c2]);
                        }


                        // Tipe 2: Pindahkan alokasi DARI (r1,c2) & (r2,c1)
                        // Donors: (r1,c2) , (r2,c1)
                        // Penerima: (r1,c1) , (r2,c2)
                        int delta2 = (cost[r1][c1] + cost[r2][c2]) - (cost[r1][c2] + cost[r2][c1]);

                        // Cek Alokasi Donor Tipe 2
                        if (alloc[r1][c2] > 0 && alloc[r2][c1] > 0) {
                             if (VERBOSE_RECTANGLE_SEARCH) {
                                printf("    [T2] Cek donor (r%d,c%d)=%d, (r%d,c%d)=%d. OK.\n",
                                       r1+1, c2+1, alloc[r1][c2], r2+1, c1+1, alloc[r2][c1]);
                                printf("    [T2] Delta = (cost[r%d][c%d] + cost[r%d][c%d]) - (cost[r%d][c%d] + cost[r%d][c%d])\n",
                                       r1+1, c1+1, r2+1, c2+1, r1+1, c2+1, r2+1, c1+1);
                                printf("    [T2] Delta = (%d + %d) - (%d + %d) = %d\n",
                                       cost[r1][c1], cost[r2][c2], cost[r1][c2], cost[r2][c1], delta2);
                            }
                            if (delta2 < best_delta) {
                                best_delta = delta2;
                                best_r1 = r1; best_c1 = c1;
                                best_r2 = r2; best_c2 = c2;
                                is_type2_move = true;
                                if (VERBOSE) {
                                    printf("    [T2] => Perbaikan T2 ditemukan! Delta baru = %d.\n", delta2);
                                }
                            } else if (VERBOSE_RECTANGLE_SEARCH) {
                                printf("    [T2] Skip: delta %d >= best_delta %d.\n", delta2, best_delta);
                            }
                        } else if (VERBOSE_RECTANGLE_SEARCH) {
                            printf("    [T2] Skip: donor (r%d,c%d)=%d atau (r%d,c%d)=%d adalah 0.\n",
                                   r1+1, c2+1, alloc[r1][c2], r2+1, c1+1, alloc[r2][c1]);
                        }
                    }
                }
            }
        }
        
        if (VERBOSE_RECTANGLE_SEARCH) {
             printf("---\n[Pencarian Selesai] Total %d rectangle dicek. Best delta ditemukan: %d\n", rect_counter, best_delta);
        }

        // Jika setelah dicek semua kemungkinan tidak ada delta negatif, optimasi selesai.
        if (best_delta >= 0) {
            if (VERBOSE) printf("\nTidak ditemukan rectangle yang memperbaiki (best_delta=%d). Optimisasi rectangle selesai.\n", best_delta);
            break; // Keluar dari loop while(true)
        }
        
        iter++;
        int r1 = best_r1, c1 = best_c1, r2 = best_r2, c2 = best_c2;
        int theta;

        if (VERBOSE) {
            printf("\n--- Perbaikan iterasi %d ---\n", iter);
            printf(" Koordinat rectangle: (r%d,c%d), (r%d,c%d)  |  tipe perbaikan: %s  |  delta=%d\n",
                   r1+1, c1+1, r2+1, c2+1, is_type2_move ? "Tipe 2" : "Tipe 1", best_delta);
            printf(" Nilai sebelum: (r%d,c%d)=%d, (r%d,c%d)=%d, (r%d,c%d)=%d, (r%d,c%d)=%d\n",
                   r1+1, c1+1, alloc[r1][c1], r1+1, c2+1, alloc[r1][c2], r2+1, c1+1, alloc[r2][c1], r2+1, c2+1, alloc[r2][c2]);
        }

        // Lakukan update alokasi berdasarkan tipe perbaikan yang ditemukan
        if (!is_type2_move) {
            // Tipe 1: Kurangi dari (r1,c1) & (r2,c2)
            theta = alloc[r1][c1] < alloc[r2][c2] ? alloc[r1][c1] : alloc[r2][c2];
            
            if (VERBOSE) {
                printf(" Tentukan theta (T1): min(alloc[r%d][c%d], alloc[r%d][c%d]) = min(%d, %d) = %d\n",
                       r1+1, c1+1, r2+1, c2+1, alloc[r1][c1], alloc[r2][c2], theta);
                printf(" Pergeseran (theta=%d):\n", theta);
                printf("  (r%d,c%d) [penerima] = %d + %d = %d\n", r1+1, c2+1, alloc[r1][c2], theta, alloc[r1][c2] + theta);
                printf("  (r%d,c%d) [penerima] = %d + %d = %d\n", r2+1, c1+1, alloc[r2][c1], theta, alloc[r2][c1] + theta);
                printf("  (r%d,c%d) [donor]    = %d - %d = %d\n", r1+1, c1+1, alloc[r1][c1], theta, alloc[r1][c1] - theta);
                printf("  (r%d,c%d) [donor]    = %d - %d = %d\n", r2+1, c2+1, alloc[r2][c2], theta, alloc[r2][c2] - theta);
            }
            if (theta <= 0) continue; 

            alloc[r1][c2] += theta;
            alloc[r2][c1] += theta;
            alloc[r1][c1] -= theta;
            alloc[r2][c2] -= theta;
        } else {
            // Tipe 2: Kurangi dari (r1,c2) & (r2,c1)
            theta = alloc[r1][c2] < alloc[r2][c1] ? alloc[r1][c2] : alloc[r2][c1];
            
             if (VERBOSE) {
                 printf(" Tentukan theta (T2): min(alloc[r%d][c%d], alloc[r%d][c%d]) = min(%d, %d) = %d\n",
                       r1+1, c2+1, r2+1, c1+1, alloc[r1][c2], alloc[r2][c1], theta);
                printf(" Pergeseran (theta=%d):\n", theta);
                printf("  (r%d,c%d) [penerima] = %d + %d = %d\n", r1+1, c1+1, alloc[r1][c1], theta, alloc[r1][c1] + theta);
                printf("  (r%d,c%d) [penerima] = %d + %d = %d\n", r2+1, c2+1, alloc[r2][c2], theta, alloc[r2][c2] + theta);
                printf("  (r%d,c%d) [donor]    = %d - %d = %d\n", r1+1, c2+1, alloc[r1][c2], theta, alloc[r1][c2] - theta);
                printf("  (r%d,c%d) [donor]    = %d - %d = %d\n", r2+1, c1+1, alloc[r2][c1], theta, alloc[r2][c1] - theta);
            }
            if (theta <= 0) continue; 

            alloc[r1][c1] += theta;
            alloc[r2][c2] += theta;
            alloc[r1][c2] -= theta;
            alloc[r2][c1] -= theta;
        }

        if (VERBOSE) {
            printf(" Nilai setelah:  (r%d,c%d)=%d, (r%d,c%d)=%d, (r%d,c%d)=%d, (r%d,c%d)=%d\n",
                   r1+1, c1+1, alloc[r1][c1], r1+1, c2+1, alloc[r1][c2], r2+1, c1+1, alloc[r2][c1], r2+1, c2+1, alloc[r2][c2]);
            long long tot = total_biaya(m, n, cost, alloc);
            printf(" Total cost setelah pembaruan = %lld\n", tot);
            print_alloc_matrix_int(m, n, alloc);
        }
    }

    if (VERBOSE) {
        printf("\n[RECTANGLE SUMMARY] Total perbaikan rectangle yang diterapkan: %d iterasi.\n", iter);
        printf("=== RECTANGLE IMPROVEMENT PHASE: END ===\n");
    }
}

static void make_feasible_ssm(int m, int n, int cost[m][n], int supply[m], int demand[n], int alloc[m][n]) {
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            alloc[i][j] = 0;

    // Inisialisasi awal: isi demand ke baris biaya terendah per kolom
    for (int j = 0; j < n; j++) {
        int rmin = 0;
        for (int i = 1; i < m; i++) if (cost[rmin][j] > cost[i][j]) rmin = i;
        alloc[rmin][j] = demand[j];
        if (VERBOSE) printf("[init] kolom c%02d -> pilih baris r%02d (cost=%d) : alokasikan %d\n",
                           j+1, rmin+1, cost[rmin][j], demand[j]);
    }

    int total_alloc_baris[m];
    int arr[m], satisfy[m];
    int flc[n], slc[n], diff[n], konflik[n];
    int itertemp = 0, jumlah_ER, coorselisih;
    const int MAX_FEAS_ITERS = 2000;

    while (1) {
        jumlah_ER = 0;
        for (int i = 0; i < m; i++) {
            arr[i] = 0;
            satisfy[i] = 0;
            total_alloc_baris[i] = 0;
            for (int j = 0; j < n; j++) total_alloc_baris[i] += alloc[i][j];
            if (total_alloc_baris[i] > supply[i]) { arr[i] = 1; jumlah_ER++; }
            else if (total_alloc_baris[i] == supply[i]) { satisfy[i] = 1; }
        }

        if (VERBOSE) {
            printf("\n=== SSM FEASIBILITY ITER %d ===\n", itertemp);
            printf("Keterangan: ER = over-supplied row, S = satisfied, NS = not-satisfied\n");
            for (int i = 0; i < m; i++) {
                for (int j = 0; j < n; j++) printf("%4d[%3d] ", cost[i][j], alloc[i][j]);
                printf(" | S=%6d TA=%6d", supply[i], total_alloc_baris[i]);
                if (arr[i]) printf("  <-- ER");
                else if (satisfy[i]) printf("  <-- S");
                else printf("  <-- NS");
                printf("\n");
            }
            printf("---------------------------------------------------------------------\n");
            print_supply_status(m, supply, total_alloc_baris);
        }

        if (jumlah_ER == 0) break;
        itertemp++;
        if (itertemp >= MAX_FEAS_ITERS) {
            printf("Peringatan: Iterasi Feasibility mencapai batas maksimum (%d)!\n", MAX_FEAS_ITERS);
            break;
        }

        // Tentukan kolom konflik (kolom yang memiliki alokasi pada row ER)
        for (int j = 0; j < n; j++) {
            konflik[j] = 0;
            for (int i = 0; i < m; i++) if (alloc[i][j] > 0 && arr[i] == 1) { konflik[j] = 1; break; }
        }

        // Tentukan flc (first conflicting row) per kolom
        for (int j = 0; j < n; j++) {
            if (!konflik[j]) { flc[j] = -1; continue; }
            flc[j] = -1;
            for (int i = 0; i < m; i++) if (alloc[i][j] > 0 && arr[i] == 1) { flc[j] = i; break; }
        }

        // Tentukan slc (second candidate row) per kolom: row dengan cost terendah yang bukan ER dan bukan SATISFY
        for (int j = 0; j < n; j++) {
            if (!konflik[j]) { slc[j] = -1; continue; }
            slc[j] = -1;
            int best = INT_MAX;
            for (int i = 0; i < m; i++) {
                if (arr[i] != 1 && satisfy[i] != 1) {
                    if (cost[i][j] < best) { best = cost[i][j]; slc[j] = i; }
                }
            }
        }

        // Hitung diff per kolom (selisih biaya slc - flc)
        for (int j = 0; j < n; j++) {
            if (konflik[j] && slc[j] != -1) diff[j] = cost[slc[j]][j] - cost[flc[j]][j];
            else diff[j] = INT_MAX / 2;
        }

        if (VERBOSE) {
            printf("Summary Kolom Konflik (hanya menampilkan kolom konflik):\n");
            printf(" col | flc(row,cost) | slc(row,cost) | diff | alokasi_di_flc\n");
            printf("-----+---------------+----------------+------+--------------\n");
            for (int j = 0; j < n; j++) {
                if (konflik[j]) {
                    printf(" c%02d | r%02d,%4d     | ", j+1, flc[j]+1, cost[flc[j]][j]);
                    if (slc[j] == -1) printf("   -,-        | ");
                    else printf(" r%02d,%4d     | ", slc[j]+1, cost[slc[j]][j]);
                    if (diff[j] >= INT_MAX / 4) printf("  -   | ");
                    else printf("%4d | ", diff[j]);
                    printf("%12d\n", (flc[j] == -1) ? 0 : alloc[flc[j]][j]);
                }
            }
        }

        // Pilih kolom dengan diff minimal (tie-breaker: alokasi lebih besar pada flc)
        coorselisih = -1;
        for (int j = 0; j < n; j++) {
            if (konflik[j] && slc[j] != -1) {
                if (coorselisih == -1 || diff[j] < diff[coorselisih]) coorselisih = j;
                else if (diff[j] == diff[coorselisih]) {
                    if (alloc[flc[j]][j] > alloc[flc[coorselisih]][coorselisih]) coorselisih = j;
                }
            }
        }

        if (coorselisih == -1) {
            if (VERBOSE) printf("Tidak ditemukan kolom konflik yang dapat dipindahkan pada iterasi ini. Keluar dari loop feasibility.\n");
            break;
        }

        int i_flc = flc[coorselisih];
        int i_slc = slc[coorselisih];

        if (VERBOSE) {
            printf("-> Kolom terpilih untuk dipindah: c%02d | flc = r%02d | slc = r%02d | diff = %d\n",
                   coorselisih+1, i_flc+1, i_slc+1, diff[coorselisih]);
            printf("   Alokasi di flc sebelum pemindahan: %d\n", alloc[i_flc][coorselisih]);
        }

        if (supply[i_flc] == supply[i_slc]) {
            int move = total_alloc_baris[i_flc] - supply[i_flc];
            if (move < 0) move = alloc[i_flc][coorselisih];
            if (move > alloc[i_flc][coorselisih]) move = alloc[i_flc][coorselisih];
            if (VERBOSE) printf("   Case: supply sama. move = %d\n", move);
            alloc[i_flc][coorselisih] -= move;
            alloc[i_slc][coorselisih] += move;
        }
        else if (jumlah_ER >= 2) {
            if (supply[i_flc] > supply[i_slc]) {
                int move = total_alloc_baris[i_flc] - supply[i_flc];
                if (move > alloc[i_flc][coorselisih]) move = alloc[i_flc][coorselisih];
                if (VERBOSE) printf("   Case: jumlah_ER>=2 and supply[flc] > supply[slc]. move = %d\n", move);
                alloc[i_flc][coorselisih] -= move;
                alloc[i_slc][coorselisih] += move;
            } else {
                int alloc_sisa = supply[i_slc] - total_alloc_baris[i_slc];
                if (VERBOSE) printf("   Case: jumlah_ER>=2 and supply[flc] <= supply[slc]. alloc_sisa = %d\n", alloc_sisa);
                if (alloc_sisa >= alloc[i_flc][coorselisih]) {
                    alloc[i_slc][coorselisih] += alloc[i_flc][coorselisih];
                    alloc[i_flc][coorselisih] = 0;
                } else {
                    alloc[i_slc][coorselisih] += alloc_sisa;
                    alloc[i_flc][coorselisih] -= alloc_sisa;
                }
            }
        } else {
            if (supply[i_flc] < supply[i_slc]) {
                int move = total_alloc_baris[i_flc] - supply[i_flc];
                if (move > alloc[i_flc][coorselisih]) move = alloc[i_flc][coorselisih];
                if (VERBOSE) printf("   Case: jumlah_ER<2 and supply[flc] < supply[slc]. move = %d\n", move);
                alloc[i_flc][coorselisih] -= move;
                alloc[i_slc][coorselisih] += move;
            } else {
                int alloc_sisa = supply[i_slc] - total_alloc_baris[i_slc];
                if (VERBOSE) printf("   Case: jumlah_ER<2 and supply[flc] >= supply[slc]. alloc_sisa = %d\n", alloc_sisa);
                if (alloc_sisa >= alloc[i_flc][coorselisih]) {
                    alloc[i_slc][coorselisih] += alloc[i_flc][coorselisih];
                    alloc[i_flc][coorselisih] = 0;
                } else {
                    alloc[i_slc][coorselisih] += alloc_sisa;
                    alloc[i_flc][coorselisih] -= alloc_sisa;
                }
            }
        }

        if (VERBOSE) {
            printf("   Setelah move -> at flc: %d, at slc: %d\n", alloc[i_flc][coorselisih], alloc[i_slc][coorselisih]);
            // print_supply_status(m, supply, total_alloc_baris);
            print_alloc_matrix_int(m, n, alloc);
            long long tot = total_biaya(m, n, cost, alloc);
            printf("   Total cost sekarang = %lld\n", tot);
        }
    }

    if (VERBOSE) {
        printf("\n=== SSM FEASIBILITY PHASE: END (iterasi = %d) ===\n", itertemp);
        print_alloc_matrix_int(m, n, alloc);
    }
}

int main(void)
{
    int sudah_optimal[36] = {0};  
    int total_optimal = 0, total_belum = 0, total_diproses = 0;

    for (int k = 1; k <= 35; k++) {
        if (optimal_solution[k] == -1) continue;
        char filename[20];
        sprintf(filename, "n%02d.txt", k);
        FILE *in = fopen(filename, "r");

        if (in == NULL) {
            continue;
        }
        
        printf("\n\n################################################################\n");
        printf("### PROCESSING FILE: %s\n", filename);
        printf("################################################################\n");

        int m, n; 
        if (fscanf(in,"%d", &m) == EOF) { fclose(in); continue; }
        fscanf(in,"%d", &n);
        
        int cost[m][n], alloc[m][n], supply[m], demand[n];
        for (int i=0;i<m;i++)
            for (int j=0;j<n;j++)
                fscanf(in,"%d",&cost[i][j]);
        for (int i=0;i<m;i++)
            fscanf(in,"%d",&supply[i]);
        for (int j=0;j<n;j++)
            fscanf(in,"%d",&demand[j]);
        fclose(in);

        if (VERBOSE) {
            printf("\n--- INPUT SUMMARY ---\n");
            printf("Dimensions : rows (m) = %d, cols (n) = %d\n", m, n);
            print_array_int("Supply", m, supply);
            print_array_int("Demand", n, demand);
            print_cost_matrix(m, n, cost, demand, supply);
            printf("----------------------\n\n");
        }

        make_feasible_ssm(m,n,cost,supply,demand,alloc);
        improve_with_rectangles(m,n,cost,alloc);

        long long tot = total_biaya(m,n,cost,alloc);
        printf("\n>>> Total Transportation Cost (final) : %lld\n", tot);

        if (tot == (long long)optimal_solution[k]) {
            printf(">>> Status: OPTIMAL (sesuai nilai yang diharapkan %d)\n", optimal_solution[k]);
            sudah_optimal[k] = 1;
            total_optimal++;
        } else {
            printf(">>> Status: BELUM OPTIMAL (seharusnya %d, diperoleh %lld)\n", optimal_solution[k], tot);
            sudah_optimal[k] = 0;
            total_belum++;
        }
        total_diproses++;
        printf("================================================================\n");
    }

    printf("\n====================== SUMMARY ======================\n");
    printf(" Total file yang diproses : %d\n", total_diproses);
    printf(" Total optimal         : %d\n", total_optimal);
    printf(" Total belum optimal   : %d\n", total_belum);
    if (total_diproses > 0) {
        double persen = (100.0 * total_optimal) / (double) total_diproses;
        printf(" Persentase optimal    : %.2f%%\n", persen);
    }

    printf("\nFile yang optimal: ");
    for (int k = 1; k <= 35; k++)
        if (optimal_solution[k] != -1 && sudah_optimal[k] == 1)
            printf("N%02d ", k);
    printf("\nFile yang belum optimal: ");
    for (int k = 1; k <= 35; k++)
        if (optimal_solution[k] != -1 && sudah_optimal[k] == 0)
            printf("N%02d ", k);
    printf("\n=====================================================\n");
    return 0;
}
