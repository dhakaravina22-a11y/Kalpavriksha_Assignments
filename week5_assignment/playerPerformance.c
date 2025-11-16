#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "Players_data.h"

#define MAX_NAME_LEN 50
#define MAX_TEAM_NAME 50
#define ROLE_COUNT 4

typedef enum {
    ROLE_UNKNOWN = 0,
    ROLE_BATSMAN = 1,
    ROLE_BOWLER = 2,
    ROLE_ALLROUNDER = 3
} Role;

typedef struct PlayerNode {
    int id;
    char name[MAX_NAME_LEN+1];
    char teamName[MAX_TEAM_NAME+1];
    Role role;
    int totalRuns;
    float battingAverage;
    float strikeRate;
    int wickets;
    float economyRate;
    double perfIndex;
    struct PlayerNode *next;
} PlayerNode;

typedef struct {
    int teamId;
    char name[MAX_TEAM_NAME+1];
    int totalPlayers;
    double sumBattingSR;
    int batterCount;
    double avgBattingStrikeRate;
    PlayerNode *playersHead;
    PlayerNode **roleArrays[ROLE_COUNT];
    int roleCounts[ROLE_COUNT];
    int roleCapacity[ROLE_COUNT];
} Team;

typedef struct {
    PlayerNode *p;
    int teamIdx;
    int idxInArray;
} HeapNode;

static Team *teamsArr = NULL;
static int gTeamCount = 0;
static void scopy(char *dest, const char *src, size_t destSize) {
    if (!dest || destSize == 0) return;
    if (!src) { dest[0] = '\0'; return; }
    strncpy(dest, src, destSize - 1);
    dest[destSize - 1] = '\0';
}
static const char *roleName(Role r) {
    switch (r) {
        case ROLE_BATSMAN: return "Batsman";
        case ROLE_BOWLER: return "Bowler";
        case ROLE_ALLROUNDER: return "All-rounder";
        default: return "Unknown";
    }
}

static Role role_from_string(const char *rstr) {
    if (!rstr) return ROLE_UNKNOWN;
    if (strcasecmp(rstr, "Batsman") == 0 || strcasecmp(rstr, "Batter") == 0) return ROLE_BATSMAN;
    if (strcasecmp(rstr, "Bowler") == 0) return ROLE_BOWLER;
    if (strcasecmp(rstr, "All-rounder") == 0 ||
        strcasecmp(rstr, "Allrounder") == 0 ||
        strcasecmp(rstr, "All rounder") == 0)
        return ROLE_ALLROUNDER;
    return ROLE_UNKNOWN;
}
static double compute_perf_index(const PlayerNode *p) {
    if (!p) return 0.0;

    if (p->role == ROLE_BATSMAN) {
        return (p->battingAverage * p->strikeRate) / 100.0;
    } else if (p->role == ROLE_BOWLER) {
        return (p->wickets * 2.0) + (100.0 - p->economyRate);
    } else if (p->role == ROLE_ALLROUNDER) {
        return ((p->battingAverage * p->strikeRate) / 100.0) + (p->wickets * 2.0);
    }
    return 0.0;
}
int read_int(const char *prompt) {
    char buffer[128];
    if (prompt) printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin))
        return -1;

    char *end;
    long val = strtol(buffer, &end, 10);
    while (isspace((unsigned char)*end)) end++;
    if (end == buffer || *end != '\0') return -1;
    return (int)val;
}

double read_double(const char *prompt) {
    char buffer[128];
    if (prompt) printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin))
        return NAN;

    char *end;
    double val = strtod(buffer, &end);
    while (isspace((unsigned char)*end)) end++;
    if (end == buffer || *end != '\0') return NAN;
    return val;
}

void read_line(char *buf, int n, const char *prompt) {
    if (prompt) printf("%s", prompt);

    if (!fgets(buf, n, stdin)) {
        buf[0] = '\0';
        return;
    }
    buf[strcspn(buf, "\n")] = '\0';
}
static int find_team_index_by_id(int teamId) {
    int low = 0, high = gTeamCount - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (teamsArr[mid].teamId == teamId) return mid;
        else if (teamsArr[mid].teamId < teamId) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

static int find_team_index_by_name(const char *tname) {
    if (!tname) return -1;
    for (int i = 0; i < gTeamCount; ++i) {
        if (strcasecmp(teamsArr[i].name, tname) == 0) return i;
    }
    return -1;
}

static int get_valid_team_index(void) {
    int tid = read_int("Enter Team ID: ");
    if (tid == -1) {
        printf("Invalid team id input.\n");
        return -1;
    }
    int idx = find_team_index_by_id(tid);
    if (idx < 0) {
        printf("Team ID %d not found.\n", tid);
        return -1;
    }
    return idx;
}
static int is_player_id_present(int pid) {
    for (int i = 0; i < gTeamCount; ++i) {
        PlayerNode *cur = teamsArr[i].playersHead;
        while (cur) {
            if (cur->id == pid) return 1;
            cur = cur->next;
        }
    }
    return 0;
}
static PlayerNode* create_player_node(int id, const char *name, const char *teamName,
                                      Role role, int runs, float avg, float sr,
                                      int wkts, float econ)
{
    PlayerNode *p = malloc(sizeof(PlayerNode));
    if (!p) { perror("malloc PlayerNode"); exit(EXIT_FAILURE); }

    p->id = id;
    scopy(p->name, name, sizeof(p->name));
    scopy(p->teamName, teamName, sizeof(p->teamName));
    p->role = role;
    p->totalRuns = runs;
    p->battingAverage = avg;
    p->strikeRate = sr;
    p->wickets = wkts;
    p->economyRate = econ;
    p->perfIndex = compute_perf_index(p);
    p->next = NULL;
    return p;
}
static void ensure_role_capacity(Team *team, int role, int want) {
    if (team->roleCapacity[role] >= want) return;

    int newcap = team->roleCapacity[role] == 0 ? 8 : team->roleCapacity[role] * 2;
    while (newcap < want) newcap *= 2;

    PlayerNode **tmp =
        realloc(team->roleArrays[role], sizeof(PlayerNode*) * newcap);

    if (!tmp) { perror("realloc roleArrays"); exit(EXIT_FAILURE); }

    team->roleArrays[role] = tmp;
    team->roleCapacity[role] = newcap;
}

static void insert_role_sorted(Team *team, int role, PlayerNode *p) {
    int n = team->roleCounts[role];
    ensure_role_capacity(team, role, n + 1);

    int pos = 0;
    while (pos < n && team->roleArrays[role][pos]->perfIndex >= p->perfIndex)
        pos++;

    for (int i = n; i > pos; --i)
        team->roleArrays[role][i] = team->roleArrays[role][i - 1];

    team->roleArrays[role][pos] = p;
    team->roleCounts[role]++;
}
static void team_add_batting_sr(Team *team, float sr) {
    team->sumBattingSR += sr;
    team->batterCount++;
    team->avgBattingStrikeRate =
    team->batterCount ? (team->sumBattingSR / team->batterCount) : 0.0;
}

static void add_player_to_team(Team *team, PlayerNode *p) {
    p->next = team->playersHead;
    team->playersHead = p;
    team->totalPlayers++;
    int r = (p->role >= ROLE_BATSMAN && p->role <= ROLE_ALLROUNDER) ?
            (int)p->role : ROLE_UNKNOWN;
    insert_role_sorted(team, r, p);

    if (p->role == ROLE_BATSMAN || p->role == ROLE_ALLROUNDER)
        team_add_batting_sr(team, p->strikeRate);
}
static void init_teams_from_header(void) {
    gTeamCount = teamCount;
    teamsArr =malloc(sizeof(Team) * gTeamCount);

    for (int i = 0; i < gTeamCount; ++i) {
        teamsArr[i].teamId = i + 1;
        scopy(teamsArr[i].name, teams[i], sizeof(teamsArr[i].name));
        teamsArr[i].totalPlayers = 0;
        teamsArr[i].sumBattingSR = 0.0;
        teamsArr[i].batterCount = 0;
        teamsArr[i].avgBattingStrikeRate = 0.0;
        teamsArr[i].playersHead = NULL;

        for (int r = 0; r < ROLE_COUNT; ++r) {
            teamsArr[i].roleArrays[r] = NULL;
            teamsArr[i].roleCounts[r] = 0;
            teamsArr[i].roleCapacity[r] = 0;
        }
    }
}

static void init_players_from_header(void) {
    for (int i = 0; i < playerCount; ++i) {
        const Player *hp = &players[i];
        Role r = role_from_string(hp->role);

        if (is_player_id_present(hp->id)) {
            fprintf(stderr,
                "Warning: duplicate player id %d for %s. Skipping.\n",
                hp->id, hp->name);
            continue;
        }

        int tidx = find_team_index_by_name(hp->team);
        if (tidx < 0) {
            fprintf(stderr,
                "Warning: team '%s' for %s not found. Skipping.\n",
                hp->team, hp->name);
            continue;
        }

        PlayerNode *p =
            create_player_node(hp->id, hp->name, hp->team, r,
                               hp->totalRuns, hp->battingAverage,
                               hp->strikeRate, hp->wickets, hp->economyRate);

        add_player_to_team(&teamsArr[tidx], p);
    }
}
static void print_players_header(int showTeam) {
    if (showTeam) {
        printf("ID\tName\t\tTeam\t\tRole\t\tRuns\tAvg\tSR\tWkts\tER\tPerfIdx\n");
    } else {
        printf("ID\tName\t\tRole\t\tRuns\tAvg\tSR\tWkts\tER\tPerfIdx\n");
    }
}

static void print_player_row(const PlayerNode *p, int showTeam) {
    if (showTeam) {
        printf("%d\t%-15s\t%-12s\t%-12s\t%d\t%.1f\t%.1f\t%d\t%.1f\t%.2f\n",
               p->id, p->name, p->teamName, roleName(p->role),
               p->totalRuns, p->battingAverage, p->strikeRate,
               p->wickets, p->economyRate, p->perfIndex);
    } else {
        printf("%d\t%-15s\t%-12s\t%d\t%.1f\t%.1f\t%d\t%.1f\t%.2f\n",
               p->id, p->name, roleName(p->role),
               p->totalRuns, p->battingAverage, p->strikeRate,
               p->wickets, p->economyRate, p->perfIndex);
    }
}

static void display_team_players(const Team *team) {
    printf("\nPlayers of Team %s (ID %d):\n", team->name, team->teamId);
    print_players_header(0);
    PlayerNode *cur = team->playersHead;
    while (cur) {
        print_player_row(cur, 0);
        cur = cur->next;
    }
    printf("Total Players: %d\n", team->totalPlayers);
    printf("Average Batting Strike Rate: %.2f\n", team->avgBattingStrikeRate);
}
static void display_top_k_players_of_team_by_role(const Team *team, Role role, int K) {
    printf("\nTop %d %s of Team %s (ID %d):\n",
           K, roleName(role), team->name, team->teamId);

    print_players_header(0);
    int n = team->roleCounts[role];
    if (n == 0) {
        printf("No players of role %s in this team.\n", roleName(role));
        return;
    }
    int limit = (K < n) ? K : n;
    for (int i = 0; i < limit; ++i)
        print_player_row(team->roleArrays[role][i], 0);
}

static int cmp_team_avg_sr_desc(const void *a, const void *b) {
    const Team *ta = *(const Team**)a;
    const Team *tb = *(const Team**)b;
    if (ta->avgBattingStrikeRate < tb->avgBattingStrikeRate) return 1;
    if (ta->avgBattingStrikeRate > tb->avgBattingStrikeRate) return -1;
    return 0;
}

static void display_teams_sorted_by_avg_sr(void) {
    Team **tmp = malloc(sizeof(Team*) * gTeamCount);
    for (int i = 0; i < gTeamCount; ++i)
        tmp[i] = &teamsArr[i];

    qsort(tmp, gTeamCount, sizeof(Team*), cmp_team_avg_sr_desc);

    printf("\nTeams Sorted by Average Batting Strike Rate:\n");
    printf("ID\tTeam Name\t\tAvg Bat SR\tTotal Players\n");
    for (int i = 0; i < gTeamCount; ++i) {
        printf("%d\t%-15s\t%.2f\t\t%d\n",
               tmp[i]->teamId, tmp[i]->name,
               tmp[i]->avgBattingStrikeRate,
               tmp[i]->totalPlayers);
    }
    free(tmp);
}
static void heap_swap(HeapNode *a, HeapNode *b) {
    HeapNode tmp = *a; *a = *b; *b = tmp;
}

static void heapify_down(HeapNode heap[], int size, int idx) {
    while (1) {
        int l = 2*idx + 1, r = 2*idx + 2;
        int largest = idx;
        if (l < size && heap[l].p->perfIndex > heap[largest].p->perfIndex)
            largest = l;
        if (r < size && heap[r].p->perfIndex > heap[largest].p->perfIndex)
            largest = r;

        if (largest != idx) {
            heap_swap(&heap[largest], &heap[idx]);
            idx = largest;
        } else break;
    }
}

static HeapNode *build_role_heap(int role, int *outSize) {
    HeapNode *heap = malloc(sizeof(HeapNode) * gTeamCount);
    int heapSize = 0;
    for (int i = 0; i < gTeamCount; ++i) {
        if (teamsArr[i].roleCounts[role] > 0) {
            heap[heapSize].p = teamsArr[i].roleArrays[role][0];
            heap[heapSize].teamIdx = i;
            heap[heapSize].idxInArray = 0;
            heapSize++;
        }
    }
    for (int i = (heapSize/2)-1; i >= 0; --i)
        heapify_down(heap, heapSize, i);

    *outSize = heapSize;
    return heap;
}

static void display_all_players_of_role_across_teams(Role role) {
    printf("\nAll players of role %s across all teams:\n", roleName(role));
    print_players_header(1);
    int heapSize = 0;
    HeapNode *heap = build_role_heap(role, &heapSize);

    while (heapSize > 0) {
        HeapNode top = heap[0];
        print_player_row(top.p, 1);
        int t = top.teamIdx;
        int nextIdx = top.idxInArray + 1;
        if (nextIdx < teamsArr[t].roleCounts[role]) {
            heap[0].p = teamsArr[t].roleArrays[role][nextIdx];
            heap[0].teamIdx = t;
            heap[0].idxInArray = nextIdx;
        } else {
            heap[0] = heap[heapSize - 1];
            heapSize--;
        }

        if (heapSize > 0)
            heapify_down(heap, heapSize, 0);
    }
    free(heap);
}
static int is_valid_name(const char *s) {
    if (!s || s[0] == '\0') return 0;
    for (int i = 0; s[i]; i++) {
        if (!isalpha((unsigned char)s[i]) && s[i] != ' ')
            return 0;
    }
    return 1;
}

static Role read_role_interactive(void) {
    int r = read_int("Role (1-Batsman, 2-Bowler, 3-All-rounder): ");
    if (r < ROLE_BATSMAN || r > ROLE_ALLROUNDER) {
        printf("Invalid role.\n");
        return ROLE_UNKNOWN;
    }
    return (Role) r;
}

static PlayerNode* read_player_data_for_team(const char *teamName) {
    char name[MAX_NAME_LEN+1];
    while (1) {
        read_line(name, sizeof(name), "Enter Player Name: ");
        if (is_valid_name(name)) break;
        printf("Invalid name! Only alphabets and spaces allowed.\n");
    }
    Role r = read_role_interactive();
    if (r == ROLE_UNKNOWN) return NULL;
    int pid;
    while (1) {
        pid = read_int("Player ID: ");
        if (pid == -1) {
            printf("Invalid input. Enter a numeric ID.\n");
            continue;
        }
        if (is_player_id_present(pid)) {
            printf("Player ID %d already exists. Try again.\n", pid);
            continue;
        }
        break;
    }
    int runs;
    while (1) {
        runs = read_int("Total Runs: ");
        if (runs >= 0) break;
        printf("Invalid runs. Try again.\n");
    }
    double avg;
    while (1) {
        avg = read_double("Batting Average: ");
        if (!isnan(avg) && avg >= 0.0) break;
        printf("Invalid average. Try again.\n");
    }
    double sr;
    while (1) {
        sr = read_double("Strike Rate: ");
        if (!isnan(sr) && sr >= 0.0) break;
        printf("Invalid strike rate. Try again.\n");
    }
    int wkts;
    while (1) {
        wkts = read_int("Wickets: ");
        if (wkts >= 0) break;
        printf("Invalid wickets. Try again.\n");
    }
    double econ;
    while (1) {
        econ = read_double("Economy Rate: ");
        if (!isnan(econ) && econ >= 0.0) break;
        printf("Invalid economy rate. Try again.\n");
    }
    return create_player_node(pid, name, teamName, r,runs, (float)avg, (float)sr,wkts, (float)econ);
}
static void free_all_memory(void) {
    if (!teamsArr) return;

    for (int i = 0; i < gTeamCount; ++i) {
        PlayerNode *cur = teamsArr[i].playersHead;
        while (cur) {
            PlayerNode *nx = cur->next;
            free(cur);
            cur = nx;
        }
        for (int r = 0; r < ROLE_COUNT; ++r) {
            free(teamsArr[i].roleArrays[r]);
        }
    }
    free(teamsArr);
    teamsArr = NULL;
}
static void action_add_player_to_team(void) {
    int tidx = get_valid_team_index();
    if (tidx < 0) return;

    PlayerNode *p = read_player_data_for_team(teamsArr[tidx].name);
    if (!p) {
        printf("Failed to create player.\n");
        return;
    }
    add_player_to_team(&teamsArr[tidx], p);
    printf("Player added successfully to Team %s!\n", teamsArr[tidx].name);
}

static void action_display_players_of_specific_team(void) {
    int tidx = get_valid_team_index();
    if (tidx >= 0)
        display_team_players(&teamsArr[tidx]);
}

static void action_display_top_k_of_team_by_role(void) {
    int tidx = get_valid_team_index();
    if (tidx < 0) return;
    Role r = read_role_interactive();
    if (r == ROLE_UNKNOWN) return;
    int K = read_int("Enter number of players (K): ");
    if (K <= 0) {
        printf("K must be positive.\n");
        return;
    }
    display_top_k_players_of_team_by_role(&teamsArr[tidx], r, K);
}

static void action_display_all_players_of_role_across_teams(void) {
    Role r = read_role_interactive();
    if (r == ROLE_UNKNOWN) return;

    display_all_players_of_role_across_teams(r);
}
static void print_menu(void) {
    printf("\nICC ODI Player Performance Analyzer\n");
    printf("1. Add Player to Team\n");
    printf("2. Display Players of a Specific Team\n");
    printf("3. Display Teams by Average Batting Strike Rate\n");
    printf("4. Display Top K Players of a Specific Team by Role\n");
    printf("5. Display all Players of a specific Role Across All Teams by performance index\n");
    printf("6. Exit\n");
    printf("Enter your choice: ");
}
int main(void) {
    init_teams_from_header();
    init_players_from_header();

    while (1) {
        print_menu();
        int choice = read_int(NULL);

        switch (choice) {
            case 1: action_add_player_to_team(); break;
            case 2: action_display_players_of_specific_team(); break;
            case 3: display_teams_sorted_by_avg_sr(); break;   // Option A (your choice)
            case 4: action_display_top_k_of_team_by_role(); break;
            case 5: action_display_all_players_of_role_across_teams(); break;
            case 6:
                free_all_memory();
                printf("Exiting. Goodbye!\n");
                return 0;

            default:
                printf("Invalid choice. Try again.\n");
        }
    }

    return 0;
}
