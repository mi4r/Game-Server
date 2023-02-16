#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

enum actions{
    buy,
    sell,
    build,
    produce,
    end
};

enum resources{
    money,
    raw,
    factories,
    production,
};

typedef struct gm{
    int lvl;
    int isStarted;
    int players;
    int month;
    int priceRaw;
    int priceProd;
    double coeffRaw;
    double coeffProd;
}
        gm;

typedef struct usr{
    int fd;
    char * buf;
    int bufsize;
    int cnt;
    int resources[4];
    int reqs[5];
    int sums[4];
    int building[4];
    int isBankrupt;
    struct usr * next;
}
        usr;

gm * init(gm * game)
{
    gm * tmp = malloc(sizeof(* tmp));
    tmp -> lvl = 3;
    tmp -> isStarted = 0;
    tmp -> players = 0;
    tmp -> priceProd = 5500;
    tmp -> priceRaw = 500;
    tmp -> coeffProd = 2.0;
    tmp -> coeffRaw = 2.0;
    tmp -> month = 0;
    return tmp;
}

void printUsrs(usr * list)
{
    usr * tmp = list;
    while(tmp){
        printf("%d %d\n", tmp -> fd, tmp -> resources[money]);
        tmp = tmp -> next;
    }
}

void printHelp(int fd)
{
    const char s0[] = ">> start                  start game";
    const char s1[] = ">> buy [num] [price]      buy [num] raw";
    const char s2[] = ">> sell [num] [price]     sell [num] production";
    const char s3[] = ">> produce [num]          produce [num] production";
    const char s4[] = ">> build [num]            build [num] factories";
    const char s5[] = ">> player [num]           show info about player#[num]";
    const char s6[] = ">> end turn               end current turn";
    const char s7[] = ">> active                 show active players";
    dprintf(fd, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",s0, s1, s2, s3, s4, s5, s6, s7);
}

void printStart(usr * users, gm * game)
{
    usr * tmp = users;
    game -> isStarted = 1;
    while(tmp){
        dprintf(tmp -> fd, ">> Game started!\n");
        tmp = tmp -> next;
    }
    printf("Game started!\n");
}

usr * findUsr(int fd , usr * list)
{
    usr * tmp = list;
    while(tmp -> fd != fd)
        tmp = tmp -> next;
    return tmp;
}

char * extendBuf(usr ** user)
{
    char * newBuf = malloc(2 * ((*user) -> cnt));
    strncpy(newBuf, (*user) -> buf, (*user) -> bufsize - 1);
    (*user) -> bufsize *= 2;
    free((*user) -> buf);
    return newBuf;
}

int countWords(char * s, int maxlen)
{
    int i = 0;
    int cnt = 0;
    while(i < maxlen){
        if(s[i] == 0)
            while(s[i] == 0)
                i++;
        cnt++;
        i++;
        while(s[i] != 0)
            i++;
    }
    return cnt - 1;
}

char ** getArgsCmd(char * buf, int len, int numW)
{
    char ** arg = malloc(numW * sizeof(* arg));
    int k;
    int i = 0;
    for(k = 0; k < numW; k++){
        while(buf[i] == 0)
            i++;
        arg[k] = &(buf[i]);
        while(buf[i] != 0)
            i++;
    }
    return arg;
}

void execNotStarted(char ** arg, usr * user, int numW, usr * list, gm * game)
{
    if(numW == 1){
        if(!strcmp("help", arg[0])){
            printHelp(user -> fd);
        }
        else if(!strcmp("start", arg[0])){
            printStart(list, game);
        }
        else
            dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");
    }
    else
        dprintf(user -> fd, ">> Incorrect input, try \"help\" OR start game\n");
}

int notEnoughMoney(int curMoney, int price, int num)
{
    return (curMoney - price * num < 0);
}

int isCorrectUser(usr * user, usr * list, int key)
{
    usr * tmp = list;
    while(tmp){
        if(key == tmp -> fd - 3)
            return 1;
        tmp = tmp -> next;
    }
    return 0;
}

void printUserInfo(usr * user, usr * list, int key)
{
    const char s1[] = ">> User #";
    const char s2[] = ">>   Money : ";
    const char s3[] = ">>   Raw : ";
    const char s4[] = ">>   Production : ";
    const char s5[] = ">>   Factories : ";
    usr * tmp = list;
    while(tmp){
        if(key == tmp -> fd - 3)
            break;
        tmp = tmp -> next;
    }
    dprintf(user -> fd, "%s%d\n%s%d\n%s%d\n%s%d\n%s%d\n", s1, key, s2,
            tmp -> resources[money], s3, tmp -> resources[raw], s4,
            tmp -> resources[production], s5, tmp -> resources[factories]);

}

int priceRange(char * arg, int req, gm * game)
{
    if(!strcmp(arg, "sell")){
        if(req <= game -> priceProd)
            return 1;
        return 0;
    }
    else{
        if(req >= game -> priceRaw)
            return 1;
        return 0;
    }
}

void printOnline(usr * users, int fd)
{
    usr * tmp = users;
    while(tmp){
        if(!tmp -> isBankrupt)
            dprintf(fd, "%d ", tmp -> fd - 3);
        tmp = tmp -> next;
    }
    dprintf(fd, "\n");
}

void execStarted(char ** arg, usr * user, int numW, usr * list, gm * game)
{
    if(numW == 1){
        if(!strcmp("active", arg[0])){
            printOnline(list, user -> fd);
        }
        else if(!strcmp("help", arg[0])){
            printHelp(user -> fd);
        }
        else if(!strcmp("start", arg[0])){
            dprintf(user -> fd, ">> Game already started\n");
        }
        else
            dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");
    }
    else if(numW == 2){
        if(!strcmp("end", arg[0])){
            if(!strcmp("turn", arg[1]))
                user -> reqs[end] = 1;
            else
                dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");
        }
        else if(!strcmp("player", arg[0])){
            int bf;
            int res = sscanf(arg[1], "%d", &bf);
            if(res == 0 || !isCorrectUser(user, list, bf))
                dprintf(user -> fd, ">> Incorrect input, try \"help\" or user doesn't exist\n");
            else
                printUserInfo(user, list, bf);
        }
        else{
            int i = 2;
            if(!strcmp("build", arg[0])){
                int res = sscanf(arg[1], "%d", &(user -> reqs[i]));
                if(res == 0 || user -> reqs[i] < 0 || notEnoughMoney(user -> resources[money], user -> sums[factories], user -> reqs[build]) || user -> reqs[end]){
                    dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");
                    user -> reqs[i] = 0;
                }
                else
                    dprintf(user -> fd, ">> Your request accepted : %s %d\n", arg[0], user -> reqs[i]);
            }
            else if(!strcmp("produce", arg[0])){
                i++;
                int res = sscanf(arg[1], "%d", &(user -> reqs[i]));
                if(res == 0 || user -> reqs[i] < 0 || user -> resources[factories] < user -> reqs[production] || notEnoughMoney(user -> resources[money], user -> sums[production], user -> reqs[produce]) || user -> resources[raw] < user -> reqs[produce] || user -> reqs[end]){
                    dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");
                    user -> reqs[i] = 0;
                }
                else
                    dprintf(user -> fd, ">> Your request accepted : %s %d\n", arg[0], user -> reqs[i]);
            }
            else
                dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");


        }
    }
    else if(numW == 3){
        int i = 0;
        if(!strcmp("buy", arg[0])){
            int res = sscanf(arg[1], "%d", &(user -> reqs[i]));
            int res2 = sscanf(arg[2], "%d", &(user -> sums[i]));
            if(res && res2 && user -> reqs[i] > 0 && user -> sums[i] > 0 && !notEnoughMoney(user -> resources[money], user -> sums[buy], user -> reqs[buy]) && !user -> reqs[end] && priceRange(arg[0], user -> sums[buy], game))
                dprintf(user -> fd, ">> Your request accepted : %s %d %d\n", arg[0], user -> reqs[i], user -> sums[i]);
            else{
                dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");
                user -> reqs[i] = 0;
            }
        }
        else if(!strcmp("sell", arg[0])){
            i++;
            int res = sscanf(arg[1], "%d", &(user -> reqs[i]));
            int res2 = sscanf(arg[2], "%d", &(user -> sums[i]));
            if(res && res2 && user -> reqs[i] > 0 && user -> sums[i] > 0 && user -> reqs[sell] <= user -> resources[production] && !user -> reqs[end] && priceRange(arg[0], user -> sums[sell], game) && !user -> isBankrupt)
                dprintf(user -> fd, ">> Your request accepted : %s %d %d\n", arg[0], user -> reqs[i], user -> sums[i]);
            else{
                user -> reqs[i] = 0;
                dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");
            }
        }
        else
            dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");
    }
    else
        dprintf(user -> fd, ">> Incorrect input, try \"help\"\n");
}

void execCmd(usr * user, gm * game, usr * list)
{
    int i;
    char ** arg;
    for(i = 0; i < user -> cnt; i++){
        if(user -> buf[i] == ' ' || user -> buf[i] == '\t' || user -> buf[i] == '\r')
            user -> buf[i] = 0;
    }
    int numWords = countWords(user -> buf, user -> cnt);
    arg = getArgsCmd(user -> buf, user -> cnt, numWords);
    if(game -> isStarted)
        execStarted(arg, user, numWords, list, game);
    else
        execNotStarted(arg, user, numWords, list, game);
}

int cmdFromPlayer(int fd, usr * list, gm ** game)
{
    usr * user = findUsr(fd, list);
    if(user -> cnt == user -> bufsize - 1)
        user -> buf = extendBuf(&user);

    int res = read(fd, &(user -> buf[user -> cnt]), 1);
    user -> cnt++;
    if(user -> buf[user -> cnt - 1] == '\n'){
        user -> buf[user -> cnt] = 0;
        execCmd(user, *game, list);
        user -> cnt = 0;
    }
    if(res == 0)
        return 0;
    return 1;
}

usr * addUsr(usr * list, int fd)
{
    int i;
    usr * tmp = malloc(sizeof(* tmp));
    tmp -> next = list;
    tmp -> fd = fd;
    tmp -> bufsize = 16;
    tmp -> cnt = 0;
    tmp -> resources[money] = 10000;
    tmp -> resources[raw] = 4;
    tmp -> resources[factories] = 2;
    tmp -> resources[production] = 2;
    tmp -> sums[production] = 2000;
    tmp -> sums[factories] = 2500;
    tmp -> isBankrupt = 0;
    for(i = 0; i < 4; i++)
        tmp -> building[i] = 0;
    for(i = 0; i < 5; i++)
        tmp -> reqs[i] = 0;
    tmp -> buf = calloc(tmp -> bufsize, 1);
    dprintf(fd, ">> Welcome, user №%d\n", fd - 3);
    return tmp;
}

usr * deleteUsr(usr * list, int fd)
{
    usr * node = list;
    if(node -> fd == fd){
        list = list -> next;
        free(node);
    }
    else{
        while(node -> next -> fd != fd)
            node = node -> next;
        if(node -> next -> next == NULL){
            free(node -> next);
            node -> next = NULL;
        }
        else{
            usr * tmp = node -> next;
            node -> next = node -> next -> next;
            free(tmp);
        }
        return node;
    }
    return list;
}

void freeMem(usr * list)
{
    usr * tmp;
    while(list){
        tmp = list;
        list = list -> next;
        free(tmp);
    }
}

int endTurn(usr * players){
    usr * tmp = players;
    int res = 1;
    while(tmp){
        res *= tmp -> reqs[end];
        tmp = tmp -> next;
    }
    return res;
}

usr * updateUsrs(usr * players, usr * disconn)
{
    usr * tmp = disconn;
    while(tmp){
        shutdown(tmp -> fd, 2);
        close(tmp -> fd);
        players = deleteUsr(players, tmp -> fd);
        tmp = tmp -> next;
    }
    freeMem(disconn);

    return players;
}

usr * changeValues(usr * users)
{
    usr * tmp = users;
    while(tmp){
        tmp -> reqs[end] = 0;
        tmp = tmp -> next;
    }
    tmp = users;
    while(tmp){
        tmp -> resources[money] -= (300 * tmp -> resources[raw] + 500 *
                                                                  tmp -> resources[production] + 1000 * tmp -> resources[factories]);
        tmp = tmp -> next;
    }
    return users;
}

int * randomize(int * arr, int left, int right)
{
    int edge = left;
    while(edge != right){
        int res = edge + rand() % (right - edge + 1);
        if(edge != res){
            arr[res] = arr[edge] ^ arr[res];
            arr[edge] = arr[edge] ^ arr[res];
            arr[res] = arr[edge] ^ arr[res];
        }
        edge++;
    }
    return arr;
}

int * eraseReqs(int maxreq, int len, int * reqs)
{
    int i;
    int maxr = maxreq;
    for(i = 0; i < len; i++){
        if(maxr - reqs[i] >= 0)
            maxr -= reqs[i];
        else if(maxr == 0){
            reqs[i] = 0;
        }
        else{
            reqs[i] -= maxr;
            maxr = 0;
        }
    }
    return reqs;
}

int compare(int a, int b, int type)
{
    if(type == sell)
        return a >= b;
    return a < b;
}

int ssort(int len, int * index, int * arr, int * reqs, int maxreq, int type)
{
    int i, j;

    for(i = 0; i < len - 1; i++){
        for (j = 0; j < len - i - 1; j++){
            if(compare(arr[j], arr[j + 1], type)){
                arr[j] = arr[j] ^ arr[j + 1];
                arr[j + 1] = arr[j] ^ arr[j + 1];
                arr[j] = arr[j] ^ arr[j + 1];
                index[j] = index[j] ^ index[j + 1];
                index[j + 1] = index[j] ^ index[j + 1];
                index[j] = index[j] ^ index[j + 1];
                reqs[j] = reqs[j] ^ reqs[j + 1];
                reqs[j + 1] = reqs[j] ^ reqs[j + 1];
                reqs[j] = reqs[j] ^ reqs[j + 1];
            }
        }
    }

    int cnt = 0;
    int key = -1;
    int begin = 0;
    while(cnt < len){
        if(key != arr[cnt]){
            if(cnt - 1 - begin > 0)
                randomize(index, begin, cnt - 1);
            begin = cnt;
            key = arr[cnt];
        }
        cnt++;
    }


    if(cnt - 1 - begin > 0)
        randomize(index, begin, cnt - 1);

    reqs = eraseReqs(maxreq, len, reqs);

    for (i = 0; i < len; i++){
        printf("#%d sum = %d req = %d\n", index[i], arr[i], reqs[i]);
    }

    return 0;
}

int findIndexById(int key, int * id)
{
    int i = 0;
    while(key != id[i])
        i++;
    return i;
}

usr * changeReqs(usr * users, int * reqs, int * id, int type)
{
    int ind;
    usr * tmp = users;
    while(tmp){
        ind = findIndexById(tmp -> fd, id);
        tmp -> reqs[type] = reqs[ind];
        tmp = tmp -> next;
    }
    return users;
}

usr * doAuction(usr * users, gm * game)
{
    int maxRaw = (int) game -> coeffRaw * game -> players;
    int maxProd = (int) game -> coeffProd * game -> players;
    int * reqs = malloc(game -> players);
    int * sums = malloc(game -> players);
    int * id = malloc(game -> players);
    usr * tmp = users;
    int i = 0;
    while(tmp){
        reqs[i] = tmp -> reqs[buy];
        sums[i] = tmp -> sums[buy];
        id[i] = tmp -> fd;
        tmp = tmp -> next;
        i++;
    }
    ssort(game -> players, id, sums, reqs, maxRaw, buy);
    users = changeReqs(users, reqs, id, buy);
    /**/
    tmp = users;
    i = 0;
    while(tmp){
        reqs[i] = tmp -> reqs[sell];
        sums[i] = tmp -> sums[sell];
        id[i] = tmp -> fd;
        tmp = tmp -> next;
        i++;
    }
    ssort(game -> players, id, sums, reqs, maxProd, sell);
    users = changeReqs(users, reqs, id, sell);
    /**/
    return users;
}

usr * trading(usr * users, gm * game)
{
    int i;
    usr * tmp = users;
    users = doAuction(users, game);
    while(tmp){
        tmp -> resources[money] -= tmp -> reqs[buy] * tmp -> sums[buy];
        tmp -> resources[raw] += (tmp -> reqs[buy] - tmp -> reqs[produce]);
        tmp -> resources[money] += tmp -> reqs[sell] * tmp -> sums[sell];
        tmp -> resources[production] += tmp -> reqs[produce] - tmp -> reqs[sell];
        tmp -> resources[money] -= tmp -> reqs[build] * tmp -> sums[build];
        tmp -> resources[factories] += tmp -> building[3];
        tmp -> resources[money] -= tmp -> reqs[produce] * tmp -> sums[produce];
        tmp -> resources[money] -= tmp -> building[3] * tmp -> sums[build];


        printf("#%d Reqs :  buy = %d sell = %d prod = %d build = %d\n", tmp -> fd - 3, tmp -> reqs[buy], tmp -> reqs[sell], tmp -> reqs[produce], tmp -> reqs[build]);

        for(i = 0; i < 5; i++)
            tmp -> reqs[i] = 0;
        for(i = 0; i < 2; i++)
            tmp -> sums[i] = 0;

        tmp = tmp -> next;
    }
    return users;
}

gm * changeLvl(gm * game)
{
    int i;
    const int rands[5][5] = {
            {4, 8, 10, 11, 12},
            {3, 7, 10, 11, 12},
            {1, 4, 8, 11, 12},
            {1, 2, 5, 9, 12},
            {1, 2, 4, 8, 12}
    };
    int res = 1 + (int)(12.0 * rand()/(RAND_MAX+1.0));
    printf("res = %d\n", res);
    int line = game -> lvl - 1;
    for(i = 0; i < 5; i++)
        if(rands[line][i] >= res){
            game -> lvl = i + 1;
            break;
        }
    /**/printf("lvl = %d\n", game -> lvl);
    return game;
}

gm * setPrices(gm * game)
{
    int prods[5] = {6500, 6000, 5500, 5000, 4500};
    int raw[5] = {800, 650, 500, 400, 300};
    double coeffRaw[5] = {1.0, 1.5, 2.0, 2.5, 3.0};
    double coeffProd[5] = {3.0, 2.5, 2.0, 1.5, 1.0};
    game -> priceProd = prods[game -> lvl - 1];
    game -> priceRaw = raw[game -> lvl - 1];
    game -> coeffRaw = coeffRaw[game -> lvl - 1];
    game -> coeffProd = coeffProd[game -> lvl - 1];
    return game;
}

int getNumUsers(usr * users)
{
    int i = 0;
    usr * tmp = users;
    while(tmp){
        if(!tmp -> isBankrupt)
            i++;
        tmp = tmp -> next;
    }
    return i;
}

usr * setBankrupts(usr * users)
{
    usr * tmp = users;
    while(tmp){
        if(tmp -> resources[money] < 0 && tmp ->isBankrupt == 0){
            tmp -> isBankrupt = 1;
            dprintf(tmp -> fd, "\n>> You are bankrupt\n\n");
        }
        tmp = tmp -> next;
    }
    return users;
}

usr * shiftBuild(usr * user)
{
    int i;
    for(i = 3; i > 0; i--)
        user -> building[i] = user -> building[i - 1];
    user -> building[0] = user -> reqs[build];
    return user;
}

usr * updateBuilds(usr * users)
{
    usr * tmp = users;
    while(tmp){
        tmp = shiftBuild(tmp);
        tmp = tmp -> next;
    }
    return users;
}

void updateGame(gm * game, usr * users)
{
    game = setPrices(game);
    game -> players = getNumUsers(users);
    users = updateBuilds(users);
    users = trading(users, game);
    users = changeValues(users);
    users = setBankrupts(users);
    game -> month++;
    game = changeLvl(game);
}

void printTurnEach(usr * players, int fd)
{
    const char s1[] = ">> User #";
    const char s2[] = ">>   Money : ";
    const char s3[] = ">>   Raw : ";
    const char s4[] = ">>   Production : ";
    const char s5[] = ">>   Factories : ";
    usr * tmp = players;
    while(tmp){
        if(!tmp -> isBankrupt)
            dprintf(fd, "%s%d\n%s%d\n%s%d\n%s%d\n%s%d\n", s1, tmp -> fd - 3, s2,
                    tmp -> resources[money], s3, tmp -> resources[raw], s4,
                    tmp -> resources[production], s5, tmp -> resources[factories]);
        printf("id = %d build : %d %d %d %d\n", tmp -> fd - 3, tmp -> building[0], tmp -> building[1], tmp -> building[2], tmp -> building[3]);
        tmp = tmp -> next;
    }
}

void printTurn(usr * players, gm * game)
{
    printf("players = %d\n", game -> players);
    usr * tmp = players;
    while(tmp){
        dprintf(tmp -> fd, ">> Active players - %d\n", game -> players);
        dprintf(tmp -> fd, ">> Current month - %d\n", game -> month);
        dprintf(tmp -> fd, ">> Current level - %d\n", game -> lvl);
        dprintf(tmp -> fd, ">> Next month min price for Raw - %d\n", game -> priceRaw);
        dprintf(tmp -> fd, ">> Next month max price for Production - %d\n", game -> priceProd);
        printTurnEach(players, tmp -> fd);
        tmp = tmp -> next;
    }
}

void handleInGame(int fd)
{
    dprintf(fd, ">> Game has started, wait for next session\n");
    shutdown(fd, 2);
    close(fd);
}

int isEndGame(gm * game, usr * users)
{
    usr * tmp = users;
    int sum = 0;
    while(tmp){
        sum += !tmp -> isBankrupt;
        tmp = tmp -> next;
    }
    if(sum == 0 || sum == 1)
        return 1;
    return 0;
}

usr * reinitU(usr * users)
{
    int i;
    usr * tmp = users;
    while(tmp){
        tmp -> resources[money] = 10000;
        tmp -> resources[raw] = 4;
        tmp -> resources[factories] = 2;
        tmp -> resources[production] = 2;
        tmp -> sums[production] = 2000;
        tmp -> sums[factories] = 2500;
        tmp -> isBankrupt = 0;
        for(i = 0; i < 4; i++)
            tmp -> building[i] = 0;
        for(i = 0; i < 5; i++)
            tmp -> reqs[i] = 0;
        tmp = tmp -> next;
    }
    return users;
}

gm * reinitG(gm * tmp)
{
    tmp -> lvl = 3;
    tmp -> isStarted = 0;
    tmp -> priceProd = 5500;
    tmp -> priceRaw = 500;
    tmp -> coeffProd = 2.0;
    tmp -> coeffRaw = 2.0;
    tmp -> month = 0;
    return tmp;
}

void endGame(gm * game, usr * users)
{
    int winner = -1;
    usr * tmp = users;
    while(tmp){
        if(!tmp -> isBankrupt)
            winner = tmp -> fd - 3;
        tmp = tmp -> next;
    }
    tmp = users;
    while(tmp){
        if(winner != -1)
            dprintf(tmp -> fd, ">> WINNER - User #%d\n", winner);
        else
            dprintf(tmp -> fd, ">> Nobody wins\n");
        tmp = tmp -> next;
    }
    game = reinitG(game);
    users = reinitU(users);
}

void processing(int ls)
{
    int max_d, fd, res;
    gm * game = NULL;
    usr * players = NULL;
    usr * tmp = NULL;
    usr * disconn = NULL;
    game = init(game);
    while(1){
        fd_set readfds;
        max_d = ls;
        FD_ZERO(&readfds);
        FD_SET(ls, &readfds);
        tmp = players;
        while(tmp){
            fd = tmp -> fd;
            FD_SET(fd, &readfds);
            if(fd > max_d)
                max_d = fd;
            tmp = tmp -> next;
        }
        res = select(max_d + 1, &readfds, NULL, NULL, NULL);
        if(res < 1)
            perror(">> Err in select()\n");
        if(FD_ISSET(ls, &readfds)){
            fd = accept(ls, NULL, NULL);
            if (fd != -1){
                if(game -> isStarted == 0)
                    players = addUsr(players, fd);
                else
                    handleInGame(fd);
            }
            else
                perror(">> Err in accept()\n");
        }
        tmp = players;
        disconn = NULL;
        while(tmp){
            fd = tmp -> fd;
            if(FD_ISSET(fd, &readfds)){
                res = cmdFromPlayer(fd, players, &game);
                if(res == 0){
                    disconn = addUsr(disconn, fd);
                    printf("User %d disconnected...\n", fd - 3);
                }
            }
            tmp = tmp -> next;
        }
        if(endTurn(players)){
            updateGame(game, players);
            printTurn(players, game);
        }
        players = updateUsrs(players, disconn);
        if(isEndGame(game, players) && game -> isStarted)
            endGame(game, players);
    }
}

int main(int argc, char ** argv)
{
    int opt = 1;
    int ls = socket(AF_INET, SOCK_STREAM, 0);
    if(argc != 2){
        perror(">> Need only 1 arg = port\n");
        exit(1);
    }
    int port = atoi(argv[1]);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (0 != bind(ls, (struct sockaddr *) &addr, sizeof(addr)))
    {
        perror(">> Smth goes wrong...\n");
        exit(1);
    }
    if (-1 == listen(ls, 5)) {
        perror(">> Smth goes wrong...\n");
        exit(1);
    }
    processing(ls);
    return 0;
}
