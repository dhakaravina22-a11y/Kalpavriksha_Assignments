#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "Players_data.h"

#define MAX_NAME_LEN 50
#define MAX_TEAM_NAME 50

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
    char name[MAX_TEAM_NAME];
    int totalPlayers;
    double avgBattingStrikeRate; 
    PlayerNode *playersHead; 
    PlayerNode **roleArrays[4];
    int roleCounts[4];     
    int roleCapacity[4];     
} Team;

typedef struct {
    PlayerNode *p;
    int teamIdx;
    int idxInArray;
} HeapNode;


static const char *roleName(int r) {
    if (r == ROLE_BATSMAN) return "Batsman";
    if (r == ROLE_BOWLER) return "Bowler";
    if (r == ROLE_ALLROUNDER) return "All-rounder";
    return "Unknown";
}


static void scopy(char *dest, const char *src, int n) {
    if (!src) { dest[0] = '\0'; return; }
    strncpy(dest, src, n-1);
    dest[n-1] = '\0';
}

static Role role_from_string(const char *rstr) {
    if (!rstr) return ROLE_UNKNOWN;
    if (strcasecmp(rstr, "Batsman") == 0 || strcasecmp(rstr, "Batter") == 0) return ROLE_BATSMAN;
    if (strcasecmp(rstr, "Bowler") == 0) return ROLE_BOWLER;
    if (strcasecmp(rstr, "All-rounder") == 0 || strcasecmp(rstr, "Allrounder") == 0 || strcasecmp(rstr, "All rounder")==0) return ROLE_ALLROUNDER;
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


static Team *teamsArr = NULL; 
static int gTeamCount = 0;

static void init_teams_from_header() {
    gTeamCount = teamCount;
    teamsArr = (Team*) malloc(sizeof(Team) * gTeamCount);
    if (!teamsArr) {
        perror("malloc teamsArr");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < gTeamCount; ++i) {
        teamsArr[i].teamId = i + 1; 
        scopy(teamsArr[i].name, teams[i], MAX_TEAM_NAME);
        teamsArr[i].totalPlayers = 0;
        teamsArr[i].avgBattingStrikeRate = 0.0;
        teamsArr[i].playersHead = NULL;
        for (int r = 0; r < 4; ++r) {
            teamsArr[i].roleArrays[r] = NULL;
            teamsArr[i].roleCounts[r] = 0;
            teamsArr[i].roleCapacity[r] = 0;
        }
    }
    
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
    for (int i = 0; i < gTeamCount; ++i) {
        if (strcasecmp(teamsArr[i].name, tname) == 0) return i;
    }
    return -1;
}

static int is_player_id_present(int pid) {
    if (!teamsArr) return 0;
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
                                      int wkts, float econ) {
    PlayerNode *p = (PlayerNode*) malloc(sizeof(PlayerNode));
    if (!p) { perror("malloc PlayerNode"); exit(EXIT_FAILURE); }
    p->id = id;
    scopy(p->name, name, MAX_NAME_LEN);
    scopy(p->teamName, teamName, MAX_TEAM_NAME);
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
    PlayerNode **tmp = (PlayerNode**) realloc(team->roleArrays[role], sizeof(PlayerNode*) * newcap);
    if (!tmp) {
        perror("realloc roleArrays");
        exit(EXIT_FAILURE);
    }
    team->roleArrays[role] = tmp;
    team->roleCapacity[role] = newcap;
}

static void insert_into_role_array_sorted(Team *team, int role, PlayerNode *p) {
    int n = team->roleCounts[role];
    ensure_role_capacity(team, role, n+1);
    int pos = 0;
    while (pos < n && team->roleArrays[role][pos]->perfIndex >= p->perfIndex) pos++;
    
    for (int i = n; i > pos; i--) {
        team->roleArrays[role][i] = team->roleArrays[role][i-1];
    }
    team->roleArrays[role][pos] = p;
    team->roleCounts[role]++;
}

static void recompute_team_avg_sr(Team *team) {
    double sum = 0.0;
    int count = 0;
    for (int r = 1; r <= 3; ++r) {
        if (r == ROLE_BOWLER) continue;
        for (int i = 0; i < team->roleCounts[r]; ++i) {
            sum += team->roleArrays[r][i]->strikeRate;
            count++;
        }
    }
    if (count == 0) team->avgBattingStrikeRate = 0.0;
    else team->avgBattingStrikeRate = sum / count;
}

static void add_player_to_team(Team *team, PlayerNode *p) {
    p->next = team->playersHead;
    team->playersHead = p;
    team->totalPlayers++;

    if (p->role >= ROLE_BATSMAN && p->role <= ROLE_ALLROUNDER) {
        insert_into_role_array_sorted(team, p->role, p);
    } else {
        insert_into_role_array_sorted(team, ROLE_BATSMAN, p);
    }
    recompute_team_avg_sr(team);
}

static void init_players_from_header() {
    for (int i = 0; i < playerCount; ++i) {
        const Player *hp = &players[i]; 
        Role r = role_from_string(hp->role);
        
        if (is_player_id_present(hp->id)) {
            fprintf(stderr, "Warning: duplicate player id %d for player %s. Skipping.\n", hp->id, hp->name);
            continue;
        }
        
        int tidx = find_team_index_by_name(hp->team);
        if (tidx < 0) {
            fprintf(stderr, "Warning: team '%s' for player %s not found. Skipping.\n", hp->team, hp->name);
            continue;
        }
        PlayerNode *p = create_player_node(hp->id, hp->name, hp->team, r,
                                           hp->totalRuns, hp->battingAverage, hp->strikeRate,
                                           hp->wickets, hp->economyRate);
        add_player_to_team(&teamsArr[tidx], p);
    }
}



static void print_player_columns_header(int showTeam) {
    if (showTeam)
        printf("ID\tName\t\tTeam\t\tRole\t\tRuns\tAvg\tSR\tWkts\tER\tPerfIdx\n");
    else
        printf("ID\tName\t\tRole\t\tRuns\tAvg\tSR\tWkts\tER\tPerfIdx\n");
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
    print_player_columns_header(0);
    PlayerNode *cur = team->playersHead;
    while (cur) {
        print_player_row(cur, 0);
        cur = cur->next;
    }
    printf("Total Players: %d\n", team->totalPlayers);
    printf("Average Batting Strike Rate: %.2f\n\n", team->avgBattingStrikeRate);
}

static void display_top_k_players_of_team_by_role(const Team *team, Role role, int K) {
    printf("\nTop %d %s of Team %s (ID %d):\n", K, roleName(role), team->name, team->teamId);
    print_player_columns_header(0);
    int n = team->roleCounts[role];
    if (n == 0) {
        printf("No players of role %s in this team.\n", roleName(role));
        return;
    }
    int limit = K < n ? K : n;
    for (int i = 0; i < limit; ++i) {
        print_player_row(team->roleArrays[role][i], 0);
    }
}

static int cmp_team_avg_sr_desc(const void *a, const void *b) {
    const Team *ta = *(const Team**)a;
    const Team *tb = *(const Team**)b;
    if (ta->avgBattingStrikeRate < tb->avgBattingStrikeRate) return 1;
    if (ta->avgBattingStrikeRate > tb->avgBattingStrikeRate) return -1;
    return 0;
}

static void display_teams_sorted_by_avg_sr() {
    // create array of pointers for sorting
    Team **tmp = (Team**) malloc(sizeof(Team*) * gTeamCount);
    if (!tmp) { perror("malloc tmp"); return; }
    for (int i = 0; i < gTeamCount; ++i) tmp[i] = &teamsArr[i];
    qsort(tmp, gTeamCount, sizeof(Team*), cmp_team_avg_sr_desc);
    printf("\nTeams Sorted by Average Batting Strike Rate:\n");
    printf("ID\tTeam Name\t\tAvg Bat SR\tTotal Players\n");
    for (int i = 0; i < gTeamCount; ++i) {
        printf("%d\t%-15s\t%.2f\t\t%d\n",
               tmp[i]->teamId, tmp[i]->name, tmp[i]->avgBattingStrikeRate, tmp[i]->totalPlayers);
    }
    free(tmp);
}

static void heap_swap(HeapNode *a, HeapNode *b) {
    HeapNode tmp = *a; *a = *b; *b = tmp;
}
static void heapify_up(HeapNode heap[], int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (heap[parent].p->perfIndex < heap[idx].p->perfIndex) {
            heap_swap(&heap[parent], &heap[idx]);
            idx = parent;
        } else break;
    }
}
static void heapify_down(HeapNode heap[], int size, int idx) {
    while (1) {
        int l = 2*idx + 1, r = 2*idx + 2, largest = idx;
        if (l < size && heap[l].p->perfIndex > heap[largest].p->perfIndex) largest = l;
        if (r < size && heap[r].p->perfIndex > heap[largest].p->perfIndex) largest = r;
        if (largest != idx) { heap_swap(&heap[largest], &heap[idx]); idx = largest; }
        else break;
    }
}


static void display_all_players_of_role_across_teams(Role role) {
    int totalN = 0;
    for (int i = 0; i < gTeamCount; ++i) totalN += teamsArr[i].roleCounts[role];
    if (totalN == 0) {
        printf("\nNo players of role %s across all teams.\n\n", roleName(role));
        return;
    }

    HeapNode *heap = (HeapNode*) malloc(sizeof(HeapNode) * gTeamCount);
    if (!heap) { perror("malloc heap"); return; }
    int heapSize = 0;
    for (int i = 0; i < gTeamCount; ++i) {
        if (teamsArr[i].roleCounts[role] > 0) {
            heap[heapSize].p = teamsArr[i].roleArrays[role][0];
            heap[heapSize].teamIdx = i;
            heap[heapSize].idxInArray = 0;
            heapSize++;
        }
    }
    for (int i = (heapSize/2)-1; i >=0; --i) heapify_down(heap, heapSize, i);

    printf("\nAll players of role %s across all teams (sorted by Perf.Index desc):\n", roleName(role));
    printf("ID\tName\t\tTeam\t\tRole\tRuns\tAvg\tSR\tWkts\tER\tPerfIdx\n");
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
        if (heapSize > 0) heapify_down(heap, heapSize, 0);
    }
    free(heap);
}

static int read_int() {
    int x;
    if (scanf("%d", &x) != 1) { while (getchar() != '\n'); return -1; }
    while (getchar() != '\n');
    return x;
}

static double read_double() {
    double d;
    if (scanf("%lf", &d) != 1) { while (getchar() != '\n'); return 0.0; }
    while (getchar() != '\n');
    return d;
}

static void read_line(char *buf, int n) {
    if (!fgets(buf, n, stdin)) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
}


static void action_add_player_to_team() {
    printf("\nEnter Team ID to add player: ");
    int tid = read_int();
    int tindex = find_team_index_by_id(tid);
    if (tindex < 0) { printf("Team ID %d not found.\n", tid); return; }

    char name[MAX_NAME_LEN];
    printf("Enter Player Name: ");
    read_line(name, MAX_NAME_LEN);

    printf("Role (1-Batsman, 2-Bowler, 3-All-rounder): ");
    int r = read_int();
    if (r < 1 || r > 3) { printf("Invalid role.\n"); return; }

    printf("Player ID: ");
    int pid = read_int();

    if (pid < 0) { printf("Invalid player id.\n"); return; }
    if (is_player_id_present(pid)) { printf("Player ID %d already exists. Cannot add duplicate.\n", pid); return; }

    printf("Total Runs: ");
    int runs = read_int();
    printf("Batting Average: ");
    double avg = read_double();
    printf("Strike Rate: ");
    double sr = read_double();
    printf("Wickets: ");
    int wkts = read_int();
    printf("Economy Rate: ");
    double econ = read_double();

    PlayerNode *p = create_player_node(pid, name, teamsArr[tindex].name, (Role)r,
                                       runs, (float)avg, (float)sr, wkts, (float)econ);
    add_player_to_team(&teamsArr[tindex], p);
    printf("Player added successfully to Team %s!\n\n", teamsArr[tindex].name);
}

static void action_display_players_of_specific_team() {
    printf("\nEnter Team ID: ");
    int tid = read_int();
    int tindex = find_team_index_by_id(tid);
    if (tindex < 0) { printf("Team ID %d not found.\n", tid); return; }
    display_team_players(&teamsArr[tindex]);
}

static void action_display_teams_by_avg_sr() {
    display_teams_sorted_by_avg_sr();
}

static void action_display_top_k_of_team_by_role() {
    printf("\nEnter Team ID: ");
    int tid = read_int();
    int tindex = find_team_index_by_id(tid);
    if (tindex < 0) { printf("Team ID %d not found.\n", tid); return; }
    printf("Enter Role (1-Batsman, 2-Bowler, 3-All-rounder): ");
    int r = read_int();
    if (r < 1 || r > 3) { printf("Invalid role.\n"); return; }
    printf("Enter number of players (K): ");
    int K = read_int();
    if (K <= 0) { printf("K must be positive.\n"); return; }
    display_top_k_players_of_team_by_role(&teamsArr[tindex], (Role)r, K);
}

static void action_display_all_players_of_role_across_teams() {
    printf("\nEnter Role (1-Batsman, 2-Bowler, 3-All-rounder): ");
    int r = read_int();
    if (r < 1 || r > 3) { printf("Invalid role.\n"); return; }
    display_all_players_of_role_across_teams((Role)r);
}

static void free_all_memory() {
    if (!teamsArr) return;
    for (int i = 0; i < gTeamCount; ++i) {
        PlayerNode *cur = teamsArr[i].playersHead;
        while (cur) {
            PlayerNode *nx = cur->next;
            free(cur);
            cur = nx;
        }
        for (int r = 0; r < 4; ++r) {
            if (teamsArr[i].roleArrays[r]) free(teamsArr[i].roleArrays[r]);
        }
    }
    free(teamsArr);
    teamsArr = NULL;
}


static void print_menu() {
    printf("ICC ODI Player Performance Analyzer\n");
    printf("1. Add Player to Team\n");
    printf("2. Display Players of a Specific Team\n");
    printf("3. Display Teams by Average Batting Strike Rate\n");
    printf("4. Display Top K Players of a Specific Team by Role\n");
    printf("5. Display all Players of a specific Role Across All Teams by performance index\n");
    printf("6. Exit\n");
    printf("Enter your choice: ");
}

int main() {
    init_teams_from_header();
    init_players_from_header();

    int choice = 0;
    while (1) {
        print_menu();
        choice = read_int();
        switch (choice) {
            case 1: action_add_player_to_team(); break;
            case 2: action_display_players_of_specific_team(); break;
            case 3: action_display_teams_by_avg_sr(); break;
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
