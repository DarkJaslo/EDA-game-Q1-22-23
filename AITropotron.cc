#include "Player.hh"
#include <map>
using namespace std;

#define PLAYER_NAME Tropotron

//Un resumen de la estrategia está escrito al final

struct PLAYER_NAME : public Player {
static Player* factory () {
  return new PLAYER_NAME;
}

//PRIORITIES

const double BATTLE_RATIO = double(1)/2;    //(N)/(N+M)
//const int DIST_FOOD_FAR = 15;
const int DIST_FOOD_FAR = 25;           //Food is too far, don't go for it
const int DIST_FOOD_ENEMY_CLOSE = 1;    //Go for foods with enemies at most closer to them by this amount
const int DIST_ZOMBIE_FAR = 25;         //Don't go for zombies if they are this far away
const int DIST_GOTODEAD = 2;            //Same as DIST_FOOD_ENEMY_CLOSE with dead units
const int DIST_TOOMUCHBFS = 30;         //Easy BFS limiter for a really irrelevant BFS
const int DIST_TOOMUCHCPU = 40;         //BFS limiter for an even more irrelevant BFS

int UNITS_TOO_LITTLE = 0;   //Unused
const int TARGET_CONFIG = 1;  //Max number of targets to the same unit
const int COMBAT_MOREUNITS = 4;   //Will wait at distance 2 if the enemy has COMBAT_MOREUNITS more than me

//Priorities: low number->low priority; high number->high priority.
const int PRIO_WORST = 0;
const int PRIO_OUT = -2;
const int PRIO_WASTE = -1;
const int PRIO_COMBAT_LOW = 1;
const int PRIO_WAITZOMBIE_LOW = 2;
const int PRIO_DANGERZOMBIE = 3;
const int PRIO_DANGER = 3;
const int PRIO_DEAD_NEXT = 4;
const int PRIO_NEUTRAL = 5;
const int PRIO_NOSTR = 10;
const int PRIO_SEARCHFOOD_FAR = 10;
const int PRIO_CELL_ME = 11;
const int PRIO_CELL_FREE = 12;
const int PRIO_CELL_ENEMY = 13;
const int PRIO_CELL_ENEMYHIGH = 14;
const int PRIO_SEARCHFOOD = 14;
const int PRIO_HIGH = 15;
const int PRIO_ZOMBIENEAR_LESS = 16;
const int PRIO_ZOMBIENEAR_ME = 17;
const int PRIO_ZOMBIENEAR_FREE = 18;
const int PRIO_ZOMBIENEAR_ENEMY = 19;
const int PRIO_ZOMBIENEAR = 20;
const int PRIO_SAFE = 21;
const int PRIO_COMBAT = 16;
const int PRIO_WAITZOMBIE = 26;
const int PRIO_FOODHERE = 27;
const int PRIO_KILLLOSE = 29;
const int PRIO_KILLZOMBIE = 30;
const int PRIO_KILLWIN = 31;

const vector<Dir> DIRS = {Left,Right,Up,Down};


//STRUCTS

//Stores info for a simple BFS
struct InfoCell{
  Pos pact;
  Pos pant;
  int dist;
  InfoCell(){
    pact = Pos(0,0);
    pant = Pos(0,0);
    dist = 0;
  }
  InfoCell(Pos act,Pos ant, int d){
    pact = act; pant = ant; dist = d;
  }
};
//Stores info about a position and its priority
struct InfoMove{
  int prio;
  Pos pos;
  InfoMove(){
    prio = 3;
    pos = Pos(0,0);
  }
  InfoMove(int priority, Pos p){
    prio = priority;
    pos = p;
  }
};
//Stores needed elements for an order
struct Order{
  int prio;
  Dir dir;
  bool canChange; //True if it can be overwritten by something else
  bool moves;     //True if it causes a unit to move
  int dist;
  Order(){
    prio = 5;
    dir = Up;
    canChange = true;
    moves = true;
    dist = -1;
  }
  Order(int pr, Dir d, bool ch, bool mov, int di){
    prio = pr; dir = d; canChange = ch; moves = mov; dist = di;
  }
};
//Stores needed information for a spot with food
struct FoodSpot{
  int id;     //Best candidate 
  int id2;    //Secondary candidate
  bool taken; //True if taken by a unit
  int dist;
  int dist2;
  int distEnemy;  //To see if an enemy is closer to it than an ally
  Pos step;   //Pos the unit id has to walk to if they want to get closer to this food
  Pos step2;  //Same thing for id2
  FoodSpot(){
    id = -1;
    id2 = -1;
    taken = false;
    dist = -1;
    dist2 = -1;
    distEnemy = -1;
    step = Pos(0,0);
  }
  FoodSpot(int ide, int ide2, bool tk, int dt, int dt2, int dtE, Pos p, Pos p2){
    id = ide; id2 = ide2; taken = tk; dist = dt; dist2 = dt2; distEnemy = dtE; step = p; step2 = p2;
  }
};
//Stores needed information for a BFS starting from a unit (visited cells and cells that have to be visited)
struct BFSstate{
  set<Pos> vis;
  queue<InfoCell> toVis;
  BFSstate(){
  }
  BFSstate(set<Pos> v, queue<InfoCell> tv){
    vis = v; toVis = tv;
  }
};

vector<int> alive;          //Stores alive units
vector<int> zombie_list;    //UNUSED
vector<vector<char>> board; //UNUSED
vector<char> waited;        //Bitmap that stores for every unit if they have waited
vector<char> waitedAnt;     //Same thing for the previous round
vector<char> ordered;       //Bitmap that stores for every unit if they have been ordered
vector<char> foodmap;       //Bitmap that stores for every unit if it's going for food
map<Pos,int> targeted;      //Stores targeted positions and the amount of targets they have
set<Pos> targetedMelee;     //UNUSED (I think)
map<int,int> aliveByPlayer; //Stores alive units by player
int size = 0;               //Used to create bitmaps


//Utility functions. I use them as macros.

/*  Pre: 0 <= n <= 7
    Returns 2^n*/
char twoToThe(int n){
  //Returns 2^n for 0 <= n <= 7. Not recursive. Yes.
  char res = 1;
  for(int i = 1; i <= n; ++i){
    res*= 2;
  }
  return res;
}
/*  Pre: posAct is adjacent to posNew
    Returns the direction Dir that goes from posAct to posNew */
Dir posToDir(Pos posAct, Pos posNew){
  int x = posNew.i -posAct.i;
  int y = posNew.j - posAct.j;
  if(x == 0){
    if(y == 1) return Right;
    else return Left;
  }
  else if(x == 1){
    return Down;
  }
  else if(x == -1){
    return Up;
  }
  return DIRS[random(0,3)];
}
/* Returns true if p is in v.
   Returns false and inserts p in v if p is not in v. */
bool isVisited(Pos p, set<Pos>& v){
  set<Pos>::const_iterator it = v.find(p);
  if(it == v.end()){
    v.insert(p);
    return false;
  }
  return true;
}
/* I believe this is not used*/
bool isTargetedMelee(Pos p){
  auto it = targetedMelee.find(p);
  return it != targetedMelee.end();
}
/*  Returns true if position p is targeted by at least one unit */
bool isTargeted(Pos p){
  auto it = targeted.find(p);
  return it != targeted.end();
}
/*  Returns the number of units that have targeted position p */
int targetNum(Pos p){
  auto it = targeted.find(p);
  if(it == targeted.end()){
    return 0;
  }
  return it->second;
}
/*  Targets/increases the number of targets of p*/
void target(Pos p){
  auto it = targeted.find(p);
  if(it == targeted.end()){
    targeted.insert(make_pair(p,1));
  }
  else{
    ++it->second;
  }
}
/*  Untargets/decreases the number of targets of p*/
void untarget(Pos p){
  auto it = targeted.find(p);
  if(it == targeted.end()) return;
  if(--it->second == 0){
    targeted.erase(it);
  }
}
/*  Returns true if act is a cell that can be visited.
    Inserts position act in visited. */
bool validCell(Pos act, Dir dir, set<Pos>& visited){
  if(pos_ok(act+dir)){
    Cell c = cell(act+dir);
    return c.type != Waste and (c.id == -1 or unit(c.id).type != Dead) and 
          not isVisited(act+dir,visited);
  }
  return false;
}
/*  Returns true if act is a cell that can be visited. This one doesn't look for visits nor insert anything*/
bool validCellNoVis(Pos p){
  if(pos_ok(p)){
    Cell c = cell(p);
    return c.type != Waste and (c.id == -1 or unit(c.id).type != Dead);
  }
  return false;
}
/*  Returns true if act is a cell that can be visited or has a dead unit in it.*/
bool validCellNoDead(Pos act, Dir dir, set<Pos>& visited){
  if(pos_ok(act+dir)){
    Cell c = cell(act+dir);
    return c.type != Waste and not isVisited(act+dir,visited);
  }
  return false;
}
/*  Targets p if an alive unit is adjacent to it */
void targetAdjEnemy(Pos p){
  vector<int> v = random_permutation(4);
  for(int i = 0; i < 4; ++i){
    Dir d = DIRS[v[i]];
    Pos p2 = p+d;
    Cell c;
    if(pos_ok(p2)){
      c = cell(p2);
      if(not c.is_empty() and c.type != Waste and not c.food and unit(c.id).type == Alive){
        target(p);
        return;
      }
    }
  }
}
/*  Pre: 0 <= n <= b.size()*8
    Returns true if the nth bit of a bitmap represented by a vector<char> is 1.*/
bool isSet(int n, vector<char>& b){
  int offset = n%8;
  int which = n/8;
  return (b[which] & twoToThe(offset));
}
/*  Pre: 0 <= n <= b.size()*8
    Sets the nth bit of the bitmap b to 1 */
void setBit(int n, vector<char>& b){
  int offset = n%8;
  int which = n/8;
  b[which] = (b[which] | twoToThe(offset));
}
/*  Pre: 0 <= n <= b.size()*8
    Clears the nth bit of the bitmap b to 0. */
void clearBit(int n, vector<char>& b){
  int offset = n%8;
  int which = n/8;
  b[which] = (b[which] & twoToThe(offset));
}
/*  Returns if there is a unit controlled by me() in p */
bool isPlayer(Pos p){
  if(not pos_ok(p)) return false;
  Cell c = cell(p);
  return not c.is_empty() and not c.food and unit(c.id).player == me();

}
/*  Queues all the valid adjacent positions in q.
    Can be used for a BFS */
void queueAdj(const InfoCell& i, set<Pos>& v, queue<InfoCell>& q){
  Pos p = i.pact;
  vector<int> s = random_permutation(4);
  for(int j = 0; j < 4; ++j){
    Dir dir = DIRS[s[j]];
    Pos pos1 = p+dir;
    if(pos_ok(pos1) and cell(pos1.i,pos1.j).type != Waste and not isVisited(pos1,v)){
      q.push(InfoCell(pos1,i.pact,i.dist+1));
    }
  }
}
int max(pair<int,int> p){
  if(abs(p.first) > abs(p.second)) return p.first;
  return p.second;
}


//What I call "more useful functions".

/*  Returns true if o can be reached from pini in distance d, and false otherwise */
bool equivalentPath(int d, Pos o, Pos pini){
  set<Pos> visited;
  visited.insert(pini);
  queue<InfoCell> toVisit;

  vector<int> v = random_permutation(4);
  for(int j = 0; j < 4; ++j){
    Dir dct = DIRS[v[j]];
    if(validCell(pini,dct,visited)){
      InfoCell i = InfoCell(pini+dct,pini+dct,1);
      toVisit.push(i);
    }
  }

  while(not toVisit.empty()){
    Pos act = toVisit.front().pact;
    Pos ant = toVisit.front().pant;
    int dist = toVisit.front().dist;
    if(dist >= d) return false;
    toVisit.pop();

    if(act == o) {
      return true;
    }

    vector<int> v = random_permutation(4);
    for(int j = 0; j < 4; ++j){
      Dir dct = DIRS[v[j]];
      if(validCell(act,dct,visited)){
        InfoCell i = InfoCell(act+dct,ant,dist+1);
        toVisit.push(i);
      }
    }
  }
  return false;
}
/*  Fills the map m with p and its up to four adjacent positions and respective priorities. */
void initMap(Pos p, map<Pos,InfoMove>& m){
  
  Pos pos1;
  InfoMove i;
  m[p] = InfoMove(PRIO_NEUTRAL,p);
  pos1 = Pos(p.i,p.j+1);
  i = InfoMove(PRIO_WORST,pos1);
  if(pos_ok(pos1)){
    if(cell(pos1.i,pos1.j).type != Waste){
      m[pos1] = i;
    }
    else{ //It's waste
      i.prio = PRIO_WASTE;
      m[pos1] = i;
    }  
  }
  else{ //It's not valid
    i.prio = PRIO_OUT;
    m[pos1] = i;
  }

  pos1 = Pos(p.i,p.j-1);
  i = InfoMove(PRIO_WORST,pos1);
  if(pos_ok(pos1)){
    if(cell(pos1.i,pos1.j).type != Waste){
      m[pos1] = i;
    }
    else{ //It's waste
      i.prio = PRIO_WASTE;
      m[pos1] = i;
    }  
  }
  else{ //It's not valid
    i.prio = PRIO_OUT;
    m[pos1] = i;
  }

  pos1 = Pos(p.i+1,p.j);
  i = InfoMove(PRIO_WORST,pos1);
  if(pos_ok(pos1)){
    if(cell(pos1.i,pos1.j).type != Waste){
      m[pos1] = i;
    }
    else{ //It's waste
      i.prio = PRIO_WASTE;
      m[pos1] = i;
    }  
  }
  else{ //It's not valid
    i.prio = PRIO_OUT;
    m[pos1] = i;
  }

  pos1 = Pos(p.i-1,p.j);
  i = InfoMove(PRIO_WORST,pos1);
  if(pos_ok(pos1)){
    if(cell(pos1.i,pos1.j).type != Waste){
      m[pos1] = i;
    }
    else{ //It's waste
      i.prio = PRIO_WASTE;
      m[pos1] = i;
    }  
  }
  else{ //It's not valid
    i.prio = PRIO_OUT;
    m[pos1] = i;
  }
}
/*  Clears and then fills the map foods with all the FoodSpots there are on the board.*/
void getFoodSpots(map<Pos,FoodSpot>& foods){
  foods.clear();
  int n = board_rows();
  int m = board_cols();
  for(int i = 0; i < n; ++i){
    for(int j = 0; j < m; ++j){
      Cell c = cell(i,j);
      if(c.food){
        FoodSpot sp;
        foods.insert(make_pair(Pos(i,j),sp));
      }
    }
  }
}
/*  Pre: p is adjacent to the current position (distance = 1)
    Updates the option p in opt taking into account different cases at distance 1, storing the one with the highest priority into it.*/
void updateOptions(Pos myp, Pos p, map<Pos,InfoMove>& opt){
  Cell c = cell(p.i,p.j);

  if(not c.is_empty()){
    if(not c.food){
      if(unit(c.id).type == Zombie){
        if(unit(cell(myp).id).rounds_for_zombie == -1 and validCellNoVis(myp+posToDir(p,myp))){
          //Checks if the zombie has allies
          int x = p.i - myp.i;
          int y = p.j - myp.j;
          bool moreZombies = false;
          if(x == 0){
            Pos paux = Pos(p.i+1,p.j);
            Pos paux2 = Pos(p.i-1,p.j);
            if(pos_ok(paux) and not cell(paux).is_empty() and not cell(paux).food and unit(cell(paux).id).type == Zombie){ //There are at least 2 zombies
              Pos paux3;
              moreZombies = true;

              //If there is a player targeting this extra zombie, no need to change behaviour
              for(int i = 0; i < 4; ++i){
                paux3 = paux+DIRS[i];
                if(isPlayer(paux3)){
                  moreZombies = false;
                }
              }
            }
            if(pos_ok(paux2) and not cell(paux2).is_empty() and not cell(paux2).food and unit(cell(paux2).id).type == Zombie){  //There are at least 2 zombies
              moreZombies = true;
              Pos paux3;
              
              //If there is a player targeting this extra zombie, no need to change behaviour
              for(int i = 0; i < 4; ++i){
                paux3 = paux2+DIRS[i];
                if(isPlayer(paux3)){
                  moreZombies = false;
                }
              }
            }
            paux = Pos(p.i,p.j+1);
            paux2 = Pos(p.i,p.j-1);
            if(pos_ok(paux) and not cell(paux).is_empty() and not cell(paux).food and unit(cell(paux).id).type == Zombie){ 
              //There are 2 adjacent zombies in a way that cannot be dealt with, just fight
              moreZombies = false;
            }
            if(pos_ok(paux2) and not cell(paux2).is_empty() and not cell(paux2).food and unit(cell(paux2).id).type == Zombie){
              //There are 2 adjacent zombies in a way that cannot be dealt with, just fight
              moreZombies = false;
            }
          }
          else{
            Pos paux = Pos(p.i,p.j+1);
            Pos paux2 = Pos(p.i,p.j-1);
            if(pos_ok(paux) and not cell(paux).is_empty() and not cell(paux).food and unit(cell(paux).id).type == Zombie){  //There are at least 2 zombies
              moreZombies = true;
              Pos paux3;

              //If there is a player targeting this extra zombie, no need to change behaviour
              for(int i = 0; i < 4; ++i){
                paux3 = paux+DIRS[i];
                if(isPlayer(paux3)){
                  moreZombies = false;
                }
              }
            }
            if(pos_ok(paux2) and not cell(paux2).is_empty() and not cell(paux2).food and unit(cell(paux2).id).type == Zombie){  //There are at least 2 zombies
              moreZombies = true;
              Pos paux3;

              //If there is a player targeting this extra zombie, no need to change behaviour
              for(int i = 0; i < 4; ++i){
                paux3 = paux2+DIRS[i];
                if(isPlayer(paux3)){
                  moreZombies = false;
                }
              }
            }
            paux = Pos(p.i+1,p.j);
            paux2 = Pos(p.i-1,p.j);
            if(pos_ok(paux) and not cell(paux).is_empty() and not cell(paux).food and unit(cell(paux).id).type == Zombie){
              //There are 2 adjacent zombies in a way that cannot be dealt with, just fight
              moreZombies = false;
            }
            if(pos_ok(paux2) and not cell(paux2).is_empty() and not cell(paux2).food and unit(cell(paux2).id).type == Zombie){
              //There are 2 adjacent zombies in a way that cannot be dealt with, just fight
              moreZombies = false;
            }
          }
          if(moreZombies){  //Extra zombies, flee

            Dir dire = posToDir(p,myp);
            Pos paux3 = myp+dire;

            if(opt[paux3].prio < PRIO_KILLZOMBIE){
              opt[paux3].prio = PRIO_KILLZOMBIE;
            }
          }
          else{ //No extra zombies, act normal
            if(opt[p].prio < PRIO_KILLZOMBIE){
              opt[p].prio = PRIO_KILLZOMBIE;
              target(p);
            }
          }
        }
        else{  //unit is becoming a zombie, probably
          if(opt[p].prio < PRIO_KILLZOMBIE){
            opt[p].prio = PRIO_KILLZOMBIE;
            target(p);
          }
        }
      }
      else if(unit(c.id).type == Alive and unit(c.id).player != me()){ //Enemy unit next
        if(strength(me()) == 0){ //Literally zero strength
          if(opt[p].prio < PRIO_NOSTR){
            opt[p].prio = PRIO_NOSTR;
          }
        }
        else if(double(strength(me()))/strength(unit(c.id).player) < BATTLE_RATIO){
          //My strength is very very very bad in comparison with the enemy's, no huge priority
          if(opt[p].prio < PRIO_KILLLOSE){
            opt[p].prio = PRIO_KILLLOSE;
          }
        }
        else{  //Try to kill as fast as possible
          if(opt[p].prio < PRIO_KILLWIN){
            opt[p].prio = PRIO_KILLWIN;
          }
        }
      }
      else if(unit(c.id).type == Dead){  //Dead unit next to it
        opt[p].prio = PRIO_DEAD_NEXT;
      }
        
    }
    else{ //Food
      if(opt[p].prio < PRIO_FOODHERE){
        opt[p].prio = PRIO_FOODHERE;
      }
    }
  }
  else{ //Empty
    if(c.owner == -1){ //Plain, white cell
      if(opt[p].prio < PRIO_CELL_FREE){
        opt[p].prio = PRIO_CELL_FREE;
      }
    }
    else if(c.owner == me()){ //My cell
      if(opt[p].prio < PRIO_CELL_ME){
        opt[p].prio = PRIO_CELL_ME;
      }
    }
    else{ //Enemy cell
      if(opt[p].prio < PRIO_CELL_ENEMY){
        opt[p].prio = PRIO_CELL_ENEMY;
      }
    }
  }
}
/*  Pre: p is at distance 2 from pIni.
    Processes movement options for important distance 2 cases in p with normal units*/
void diagonalStraight(Pos pIni, Pos p, map<Pos,InfoMove>& opt){
  int x = p.i - pIni.i;
  int y = p.j - pIni.j;
  targetAdjEnemy(p);

  if(abs(x) == abs(y) and not isTargeted(p)){ //Diagonal
    Pos run = Pos(pIni.i+x,pIni.j);
    if(opt[run].prio > PRIO_DANGERZOMBIE){
      opt[run].prio = PRIO_DANGERZOMBIE;
    } 
    Pos run2 = Pos(pIni.i,pIni.j+y);
    if(opt[run2].prio > PRIO_DANGERZOMBIE){
      opt[run2].prio = PRIO_DANGERZOMBIE;
    }

    Pos possible = Pos(pIni.i-x,pIni.j);
    if(not validCellNoVis(possible)){
      opt[possible].prio = PRIO_WASTE;
    }
    Pos possible2 = Pos(pIni.i,pIni.j-y);
    if(not validCellNoVis(possible2)){
      opt[possible2].prio = PRIO_WASTE;
    }
    int prio1 = opt[possible].prio;
    int prio2 = opt[possible2].prio;

    if(prio1 > PRIO_WASTE and not isTargeted(possible) and not isPlayer(possible)){
      Cell c = cell(possible);
      if(c.owner == -1){
        if(prio1 <= PRIO_ZOMBIENEAR_FREE or prio1 == PRIO_SAFE){
          opt[possible].prio = PRIO_ZOMBIENEAR_FREE;
          opt[pIni].prio = PRIO_DANGERZOMBIE;
        } 
      }
      else if(c.owner == me()){
        if(prio1 <= PRIO_ZOMBIENEAR_ME or prio1 == PRIO_SAFE){
          opt[possible].prio = PRIO_ZOMBIENEAR_ME;
          opt[pIni].prio = PRIO_DANGERZOMBIE;
        } 
      }
      else{
        if(prio1 <= PRIO_ZOMBIENEAR_ENEMY or prio1 == PRIO_SAFE){
          opt[possible].prio = PRIO_ZOMBIENEAR_ENEMY;
          opt[pIni].prio = PRIO_DANGERZOMBIE;
        } 
      }
    }
    else{
      opt[possible].prio = PRIO_WASTE;
    }
    
    if(prio2 > PRIO_WASTE and not isTargeted(possible2) and not isPlayer(possible2)){
      Cell c = cell(possible2);
      if(c.owner == -1){
        if(prio2 <= PRIO_ZOMBIENEAR_FREE or prio2 == PRIO_SAFE){
          opt[possible2].prio = PRIO_ZOMBIENEAR_FREE;
          opt[pIni].prio = PRIO_DANGERZOMBIE;
        } 
      }
      else if(c.owner == me()){
        if(prio2 <= PRIO_ZOMBIENEAR_ME or prio2 == PRIO_SAFE){
          opt[possible2].prio = PRIO_ZOMBIENEAR_ME;
          opt[pIni].prio = PRIO_DANGERZOMBIE;
        } 
      }
      else{
        if(prio2 <= PRIO_ZOMBIENEAR_ENEMY or prio2 == PRIO_SAFE){
          opt[possible2].prio = PRIO_ZOMBIENEAR_ENEMY;
          opt[pIni].prio = PRIO_DANGERZOMBIE;
        } 
      }
    }
    else{
      opt[possible2].prio = PRIO_WASTE;
    }   

    if(opt[possible].prio == opt[possible2].prio){
      int r = random(0,1);
      if(r == 0){
        opt[possible].prio = PRIO_ZOMBIENEAR_LESS;
      }
      else{
        opt[possible2].prio = PRIO_ZOMBIENEAR_LESS;
      }
    }
  }
  else if(not isTargeted(p)){ //Straight
    if(opt[pIni].prio != PRIO_DANGERZOMBIE and opt[pIni].prio < PRIO_SAFE) opt[pIni].prio = PRIO_SAFE;
  }
}
/*  Pre: p is at distance 2 from pIni.
    Processes movement options for important distance 2 cases in p with 'zombiefying' units*/
void diagonalStraightZombie(Pos pIni, Pos p, map<Pos,InfoMove>& opt){
  int x = p.i - pIni.i;
  int y = p.j - pIni.j;
  if(abs(x) == abs(y)){ //Diagonal
    Pos possible = Pos(pIni.i+x,pIni.j);
    int prio1 = opt[possible].prio;
    Pos possible2 = Pos(pIni.i,pIni.j+y);
    int prio2 = opt[possible2].prio;

    if(prio1 <= PRIO_WASTE or prio1 == PRIO_DEAD_NEXT){
      if(prio2 <= PRIO_ZOMBIENEAR){
        opt[possible2].prio = PRIO_ZOMBIENEAR;
      }
    }
    else if(prio2 <= PRIO_WASTE or prio2 == PRIO_DEAD_NEXT){
      if(prio1 <= PRIO_ZOMBIENEAR){
        opt[possible].prio = PRIO_ZOMBIENEAR;
      }
    }
    else{
      Cell c = cell(possible.i,possible.j);
      if(c.owner == -1){
        if(prio1 <= PRIO_ZOMBIENEAR_FREE) opt[possible].prio = PRIO_ZOMBIENEAR_FREE;
      }
      else if(c.owner == me()){
        if(prio1 <= PRIO_ZOMBIENEAR_ME) opt[possible].prio = PRIO_ZOMBIENEAR_ME;
      }
      else{
        if(prio1 <= PRIO_ZOMBIENEAR_ENEMY) opt[possible].prio = PRIO_ZOMBIENEAR_ENEMY;
      }

      c = cell(possible2.i,possible2.j);
      if(c.owner == -1){
        if(prio2 <= PRIO_ZOMBIENEAR_FREE) opt[possible2].prio = PRIO_ZOMBIENEAR_FREE;
      }
      else if(c.owner == me()){
        if(prio2 <= PRIO_ZOMBIENEAR_ME) opt[possible2].prio = PRIO_ZOMBIENEAR_ME;
      }
      else{
        if(prio2 <= PRIO_ZOMBIENEAR_ENEMY) opt[possible2].prio = PRIO_ZOMBIENEAR_ENEMY;
      }

      prio1 = opt[possible].prio;
      prio2 = opt[possible2].prio;

      if(opt[possible].prio == opt[possible2].prio){
        int r = random(0,1);
        if(r == 0){
          opt[possible].prio = PRIO_ZOMBIENEAR_LESS;
        }
        else{
          opt[possible2].prio = PRIO_ZOMBIENEAR_LESS;
        }
      }
    }
  }
  else{ //Straight
    if(opt[pIni].prio <= PRIO_SAFE) opt[pIni].prio = PRIO_SAFE;
  }
}
/*  Pre: p is at distance 2 from pini.
    Processes all important cases for distance 2 at pos p for a unit in position pini*/
void updateOptions2(Pos p, Pos pant, Pos pini, map<Pos,InfoMove>& opt){
  Cell c = cell(p.i,p.j);

  if(not c.is_empty()){
    if(not c.food){
      if(unit(c.id).type == Zombie){
        //Zombie at distance 2
        if(unit(cell(pini).id).rounds_for_zombie != -1){ //For soon-to-be zombies
          diagonalStraightZombie(pini,p,opt);
        }
        else{ //For regular units
          diagonalStraight(pini,p,opt);
        }
      }
      else if(unit(c.id).player != me() and unit(c.id).type != Dead){
        //Tries to get the last turn to "guess" the next enemy position, assuming it will come

        int id = cell(pini).id;
        if(strength(unit(cell(p).id).player) < 1.5*strength(me())){
          if(cell(pant).is_empty() and not isTargeted(pant)){
            if(unit(cell(p).id).rounds_for_zombie == -1){
              int dif = aliveByPlayer[unit(cell(p).id).player] - aliveByPlayer[me()];
              if(dif < COMBAT_MOREUNITS){
                opt[pant].prio = PRIO_COMBAT;
              }
              else{
                if(not isSet(id,waitedAnt)){
                  opt[pini].prio = PRIO_SAFE;
                  setBit(id,waited);
                }
                else{
                  opt[pant].prio = PRIO_COMBAT;
                }
              }
            }
            else{
              opt[pant].prio = PRIO_DANGER;
            }
          }
        }
        else{
          opt[pant].prio = PRIO_DANGER;
        }
        if(isSet(id,waitedAnt)) clearBit(id,waited);
      }
    }
  }
}
/*  Returns true if an option in opt is high-priority; leaves it in i.
    Returns false if no such option exists; the value in i does not matter in this case.*/
bool urgent(const map<Pos,InfoMove>& opt,InfoMove& i){
  InfoMove info;
  info.prio = PRIO_WORST;
  for(pair<Pos,InfoMove> options: opt){
    if(options.second.prio > info.prio){
      if(info.prio == PRIO_KILLZOMBIE){
        untarget(info.pos);
      }
      info = options.second;
      info.pos = options.first;
    }
    else if(options.second.prio == info.prio and info.prio == PRIO_KILLZOMBIE){
      if(targetNum(info.pos) > targetNum(options.second.pos)){
        untarget(info.pos);
        info = options.second;
        info.pos = options.first;
      }
    }
  }
  if(info.prio > PRIO_HIGH){
    i = info;
    //cerr << "  " << i.pos << "is urgent with prio " << i.prio << endl;
    return true;
  }
  return false;
}
/*  Stores p in pres if its cell has a higher priority according to the system, the value of which is too stored in pr.*/
void replaceIfBetter(Pos p, Dir dir, int& pr, Pos& pres, set<Pos>& vis){
  if(validCell(p,dir,vis)){
    Cell c = cell(p+dir);
    int prio = PRIO_WORST;
    if(not isTargeted(p+dir) and c.is_empty()){
      if(c.owner == -1){
        prio = PRIO_CELL_FREE;
      }
      else if(c.owner == me()){
        prio = PRIO_CELL_ME;
      }
      else{
        prio = PRIO_CELL_ENEMY;
        vector<int> scores(4);
        int maxScore = 0;
        for(int i = 0; i < 4; ++i){
          scores[i] = score(i);
          if(i != me() and scores[i] > maxScore) maxScore = scores[i];
        }
        if(scores[c.owner] == maxScore){
          prio = PRIO_CELL_ENEMYHIGH;
        }
        else{
          prio = PRIO_CELL_ENEMY;
        }
      }
      if(prio > pr){
        pr = prio;
        pres = p+dir;
      }
    }
  }
}

/*  For a situation like this:

.x........   x has two options if it's going for y: right or down. Going right maximizes diagonality,
..........   if down was selected (due to the random order of BFS) it calculates the distance 
.......y..   starting from right, and changes mov if it's at the same distance and diagonally better.

*/
void considerAlternative(Pos p, Pos& mov, Pos o, int d){
  pair<int,int> v;
  v.first = o.i-p.i;
  v.second = o.j-p.j;
  if(v.first == v.second) return;
  pair<int,int> v2;
  v2.first = o.i-mov.i;
  v2.second = o.j-mov.j;
  if(max(v) == max(v2)){
    Dir dir = posToDir(p,mov);
    Dir dir2;
    if(dir == Up or dir == Down){
      if(o.j - p.j < 0){
        dir2 = Left;
      }
      else dir2 = Right;
    }
    else{
      if(o.i-p.i < 0){
        dir2 = Up;
      }
      else dir2 = Down;
    }
    Pos mov2 = p+dir2;
    if(validCellNoVis(mov2) and not isTargeted(mov2) and equivalentPath(d,o,mov2)){
      mov = mov2;
      return;
    }
  }
  return;  
}
/*  Returns the best position to move to according to the priority system.
    Can return p if no valid option exists.*/
Pos bestAdjacent(Pos p){
  set<Pos> vis;
  Pos pres = p;
  int prio = PRIO_WORST;
  
  vector<int> v = random_permutation(4);
  for(int i = 0; i < 4; ++i){
    Dir dir = DIRS[v[i]];
    replaceIfBetter(p,dir,prio,pres,vis);
  }
  if(prio <= PRIO_WORST) return p;
  return pres;
}
/*  Returns the position of the nearest corpse, starting from position pini.*/
Pos nearestCorpse(Pos pini){
  set<Pos> visited;
  visited.insert(pini);
  queue<InfoCell> toVisit;

  vector<int> v = random_permutation(4);
  for(int j = 0; j < 4; ++j){
    Dir dct = DIRS[v[j]];
    //Dir dct = DIRS[j];
    if(validCell(pini,dct,visited)){
      InfoCell i = InfoCell(pini+dct,pini+dct,1);
      toVisit.push(i);
    }
  }
  
  while(not toVisit.empty()){
    Pos act = toVisit.front().pact;
    Pos ant = toVisit.front().pant;
    int dist = toVisit.front().dist;
    toVisit.pop();

    if(dist >= DIST_TOOMUCHBFS) return pini;

    Cell c = cell(act);
    if(not c.is_empty() and not c.food and unit(c.id).type == Dead) {
      return ant;
    }

    vector<int> v = random_permutation(4);
    for(int j = 0; j < 4; ++j){
      Dir dct = DIRS[v[j]];
      //Dir dct = DIRS[j];
      if(validCell(act,dct,visited)){
        InfoCell i = InfoCell(act+dct,ant,dist+1);
        toVisit.push(i);
      }
    }
  }
  return pini;
}
/*  Same thing for allies*/
Pos nearestAlly(Pos pini){
  set<Pos> visited;
  visited.insert(pini);
  queue<InfoCell> toVisit;

  vector<int> v = random_permutation(4);
  for(int j = 0; j < 4; ++j){
    Dir dct = DIRS[v[j]];
    if(validCell(pini,dct,visited)){
      InfoCell i = InfoCell(pini+dct,pini+dct,1);
      toVisit.push(i);
    }
  }

  while(not toVisit.empty()){
    Pos act = toVisit.front().pact;
    Pos ant = toVisit.front().pant;
    int dist = toVisit.front().dist;
    toVisit.pop();

    if(dist >= DIST_TOOMUCHBFS) return pini;

    Cell c = cell(act);
    if(not c.is_empty() and not c.food and unit(c.id).type == Alive and unit(c.id).player == me()) {
      return ant;
    }

    vector<int> v = random_permutation(4);
    for(int j = 0; j < 4; ++j){
      Dir dct = DIRS[v[j]];
      if(validCell(act,dct,visited)){
        InfoCell i = InfoCell(act+dct,ant,dist+1);
        toVisit.push(i);
      }
    }
  }
  return pini;
}
/*  Same thing for unpainted cells */
Pos findFree(Pos pini){
  set<Pos> visited;
  visited.insert(pini);
  queue<InfoCell> toVisit;

  vector<int> v = random_permutation(4);
  for(int j = 0; j < 4; ++j){
    Dir dct = DIRS[v[j]];
    if(validCell(pini,dct,visited)){
      InfoCell i = InfoCell(pini+dct,pini+dct,1);
      toVisit.push(i);
    }
  }

  while(not toVisit.empty()){
    Pos act = toVisit.front().pact;
    Pos ant = toVisit.front().pant;
    int dist = toVisit.front().dist;
    if(dist >= DIST_TOOMUCHCPU) break;
    toVisit.pop();

    if(cell(act).owner != me()) {
      if(not isTargeted(ant)){
        return ant;
      }
    }

    vector<int> v = random_permutation(4);
    for(int j = 0; j < 4; ++j){
      Dir dct = DIRS[v[j]];
      if(validCell(act,dct,visited)){
        InfoCell i = InfoCell(act+dct,ant,dist+1);
        toVisit.push(i);
      }
    }
  }
  return bestAdjacent(pini);
}
//Given a FoodSpot at pos p, assigns a player to it if the conditions are met
void nearestOkPlayer(Pos p, map<int,Order>& orders, map<Pos,FoodSpot>& foods, FoodSpot& sp){
  /*
  Para todo FoodSpot, tenemos 2 candidatos
  Esto sirve para que el sistema sea algo más flexible
  -Un id solo puede ser candidato principal de un foodspot
    bitmap de ids con esa propiedad
  -Si un FoodSpot pierde a su id (porque se vuelve id principal de otro)
  pasa a usar el id2 -> se marca que ahora es principal de un FoodSpot
    -Si id2 ya es principal de otro FoodSpot, se deja id a -1 con la esperanza
    de que a lo mejor se pierda ese principal por algún reemplazo
  Situaciones en las que se puede quitar un id:
    -El sp actual ha encontrado a un id que está asignado a otro del que está más lejos
    -
  -Si un id está en uso como principal, se pone de secundario (está más cerca, podemos tener suerte)
    Al final se intentarán poner todos esos id2 cercanos más cerca

  Conclusión final: esto se habría arreglado fácil con un BFS por capas desde las comidas. Una pena
  que lo diseñara después. Por suerte funciona bien esto :P
  */

  sp.taken = false;
  set<Pos> visited;
  visited.insert(p);
  queue<InfoCell> toVisit;
  bool idok = false;
  bool id2ok = false;
  bool enemyok = false;

  //Starts BFS
  vector<int> v = random_permutation(4);
  for(int j = 0; j < 4; ++j){
    Dir dct = DIRS[v[j]];
    if(validCell(p,dct,visited)){
      InfoCell i = InfoCell(p+dct,p,1);
      toVisit.push(i);
    }
  }

  while(not toVisit.empty()){
    Pos act = toVisit.front().pact;
    Pos ant = toVisit.front().pant;
    int dist = toVisit.front().dist;
    toVisit.pop();

    //Return if an enemy is much closer to it than an ally
    if(sp.distEnemy != -1){
      if((sp.dist == -1)){
        if((dist-sp.distEnemy) >= DIST_FOOD_ENEMY_CLOSE){
          sp.id = -1;
          sp.taken = true;
          return;
        }
      }
      if((sp.dist2 == -1)){
        if((dist-sp.distEnemy) >= DIST_FOOD_ENEMY_CLOSE){
          sp.id2 = -1;
          sp.taken = true;
          return;
        }
      }
    }

    Cell c = cell(act);
    if(not c.is_empty() and not c.food and unit(c.id).type != Zombie){
      Unit u = unit(c.id);
      if(u.player == me()){ //Found an ally
        Order ord = orders[c.id];
        if(/*(isSet(c.id,ordered) and*/ ord.canChange/*) or not isSet(c.id,ordered)*/){ //Order is not urgent

          if(not idok){ //id1 not assigned yet
            if(not isSet(c.id,foodmap)){ //Not the id of a FoodSpot
              sp.id = c.id;
              sp.dist = dist;
              sp.taken = true;
              sp.step = ant;
              setBit(c.id,foodmap);
              idok = true;
            }
            else{ //It is the id of a FoodSpot
              for(map<Pos,FoodSpot>::iterator it = foods.begin(); it != foods.end(); ++it){
                
                //FoodSpot taken by c.id found
                if(it->second.id == c.id){
                  if(it->second.dist > dist){ //Take id away from it
                    sp.id = c.id;
                    sp.dist = dist;
                    sp.step = ant;
                    sp.taken = true;
                    idok = true;
                    //Fixing the FoodSpot that got robbed
                    if(not isSet(it->second.id2,foodmap)){ //id2 can become id
                      it->second.id = it->second.id2;
                      it->second.dist = it->second.dist2;
                      it->second.step = it->second.step2;
                      setBit(it->second.id,foodmap);
                      it->second.id2 = -1;
                      it->second.dist2 = -1;
                      it->second.step2 = Pos(0,0);
                    }
                    else{ //id2 is the id of another FoodSpot
                      it->second.id = -1;
                      it->second.dist = -1;
                      it->second.step = Pos(0,0);
                      it->second.taken = false;
                    }
                  }
                  else{ //Do not, just save it and cope
                    if(not id2ok){
                      sp.id2 = c.id;
                      sp.dist2 = dist;
                      sp.step2 = ant;
                      id2ok = true;
                    }
                  }
                  break;
                }
              }
            }
          }
          else{ //id1 assigned
            
            sp.id2 = c.id;
            sp.dist2 = dist;
            sp.step2 = ant;
            id2ok = true;
          }
        }
      }
      else{ //Found an enemy
        if(sp.distEnemy == -1){
          sp.distEnemy = dist;
          enemyok = true;
        } 
      }
    }

    if(idok and id2ok and enemyok){
      return;
    } 

    vector<int> v = random_permutation(4);
    for(int j = 0; j < 4; ++j){
      Dir dct = DIRS[v[j]];
      if(validCell(act,dct,visited)){
        InfoCell i = InfoCell(act+dct,act,dist+1);
        toVisit.push(i);
      }
    }
  }
}
/*  Returns true and stores in z the position leading to a zombie, corpse or player
    Receives the visited set and the toVisit queue of a BFS, and returns when it finds something
    or d is exceeded. It is usually used to look for things at the previous distance+1, thus exploring each distance at a time. Useful if you want to BFS from every unit at the same time
    (I decided to call this 'layered BFS')*/
bool nearestSkirmish(int d, Pos p, Pos& z, Pos& o, set<Pos>& visited, queue<InfoCell>& toVisit){

  while(not toVisit.empty()){
    Pos act = toVisit.front().pact;
    Pos ant = toVisit.front().pant;
    int dist = toVisit.front().dist;
    if(dist >= d){
      return false;
    }
    toVisit.pop();

    Cell c = cell(act);
    if(not c.is_empty() and not c.food){ //Unit
      Unit u = unit(c.id);
      if(u.type == Dead){ //It's dead
        if(dist == 1){
          if(not isTargeted(act)){
            //The dead unit next to this unit is targeted now
            target(act);
            z = p;
            o = act;
            return true;
          }
        }
        else{
          if(targetNum(act) < TARGET_CONFIG){
            z = ant;
            o = act;
            target(act);
            return true;
          }
        }
      }
      else if(u.type == Alive and u.player != me()){
        if(not isTargeted(act)){
          //The unit this unit has found is targeted now
          target(act);
          if(static_cast<int>(alive.size()) > UNITS_TOO_LITTLE){
            z = ant;
            o = act;
            return true;
          }
        }
      }
      else if(u.type == Zombie){
        if(not isTargeted(act)){
          if(dist == 3){
            if(act.i != p.i and act.j != p.j){//Horse chess movement
              if(abs(act.i-p.i) == 1) {
                Dir direct;
                if(act.i-p.i == 1){
                  direct = Down;
                }
                else{
                  direct = Up;
                }
                Pos aux = p+direct;
                if(validCellNoVis(aux) and not isTargeted(aux)){
                  target(aux);
                  z = aux;
                  return true;
                }
                else{
                  target(p);
                  z = p;
                  return true;
                }
              }
              else{
                Dir direct;
                if(act.j-p.j == 1){
                  direct = Right;
                }
                else{
                  direct = Left;
                }
                Pos aux = p+direct;
                if(validCellNoVis(aux) and not isTargeted(aux)){
                  target(aux);
                  z = aux;
                  return true;
                }
                else{
                  target(p);
                  z = p;
                  return true;
                }
              }
            }
          }
          //No problem, zombie is on a straight line
          target(act);
          z = ant;
          return true;
        }
      }     
    }

    vector<int> v = random_permutation(4);
    for(int j = 0; j < 4; ++j){
      Dir dct = DIRS[v[j]];
      if(validCellNoDead(act,dct,visited)){
        InfoCell i = InfoCell(act+dct,ant,dist+1);
        toVisit.push(i);
      }
    }
  }
  return false;
}
/*  Stores all food-related orders; essentially calls nearestOkPlayer() for each FoodSpot.*/
void giveFoodOrders(map<Pos,FoodSpot>& foods,map<int,Order>& orders){
  for(map<Pos,FoodSpot>::iterator it = foods.begin(); it != foods.end(); ++it){
    nearestOkPlayer(it->first,orders,foods,it->second);    
  }
  
  //Recupera a los id2 que puedan convertirse en id1 (id1 está a -1)
  for(map<Pos,FoodSpot>::iterator it = foods.begin(); it != foods.end(); ++it){
    
    if(not it->second.taken){
      if(it->second.id == -1 and it->second.id2 != -1){
        if(not isSet(it->second.id2, foodmap)){
          it->second.id = it->second.id2;
          it->second.dist = it->second.dist2;
          it->second.step = it->second.step2;
          it->second.taken = true;
          setBit(it->second.id,foodmap);
        }
      }
    }
    if(it->second.taken and it->second.id != -1){  //Taken
      map<int,Order>::iterator it2 = orders.find(it->second.id);
      it2->second.dir = posToDir(unit(it->second.id).pos,it->second.step);
      it2->second.moves = true;
      /*if(it->second.dist - it->second.distEnemy <= DIST_FOOD_ENEMY_CLOSE+1){
        //If food is far enough, let yourself be influenced by chance
        it2->second.canChange = false;
      }
      else{
        //Must take the food
        it2->second.canChange = true;
      }*/
      it2->second.canChange = false;
      it2->second.prio = PRIO_SEARCHFOOD;
      //Position the unit will move to is targeted now
      target(unit(it2->first).pos+it2->second.dir);
      //FoodSpot is targeted now
      target(it->first);
    }
  }
}
/*  Modifies all non-urgent and non-food-related orders to look for zombies.
    Uses a layered BFS to assign everything correctly.*/
void giveZombieOrders(map<int,Order>& orders){
  
  map<int,BFSstate> skirmishes;

  for(auto it = orders.begin(); it != orders.end(); ++it){
    if(it->second.canChange){
      BFSstate bfs;
      Pos p = unit(it->first).pos;
      bfs.vis.insert(p);
      
      vector<int> v = random_permutation(4);
      for(int j = 0; j < 4; ++j){
        Dir dct = DIRS[v[j]];
        if(validCellNoDead(p,dct,bfs.vis)){
          InfoCell i = InfoCell(p+dct,p+dct,1);
          bfs.toVis.push(i);
        }
      }
      skirmishes.insert(make_pair(it->first,bfs));
    }
  }

  for(int i = 1; i < DIST_ZOMBIE_FAR; ++i){
    for(auto it = skirmishes.begin(); it != skirmishes.end(); ++it){
      Pos p;
      Unit u = unit(it->first);
      Pos o = u.pos;
      if(nearestSkirmish(i,u.pos,p,o,it->second.vis,it->second.toVis)){

        auto it2 = orders.find(it->first);
        if(u.pos == p){ //Dead unit next to it

          //If it has a food order but it's not necessary to go right now
          if((i == 1 and isSet(u.id,foodmap)) or not isSet(u.id,foodmap)){
            it2->second.moves = true;
            it2->second.dir = posToDir(u.pos,o);
            //The position this unit is on is targeted now
            target(p);
          }
        }
        else{

          //Choose alternate path if it exists and it's better
          if(o != u.pos){
            considerAlternative(u.pos,p,o,i);
          }

          it2->second.dir = posToDir(u.pos,p);
          it2->second.prio = PRIO_NEUTRAL;
          it2->second.moves = true;

          //The position this unit will move to is targeted now
          target(p);
        }
        it2->second.canChange = false;
        auto it3 = it;
        ++it;
        skirmishes.erase(it3);
        if(skirmishes.empty()) break;
        //--it;
      }
    }
  }

  for(auto it2 = skirmishes.begin(); it2 != skirmishes.end(); ++it2){
    auto it = orders.find(it2->first);
    if(it->second.canChange){
      Unit u = unit(it->first);
      Pos p = bestAdjacent(u.pos);
      if(p == u.pos){
        it->second.moves = false;
        //The position this unit is on is targeted now
        target(p);
      }
      else{
        if(cell(p).owner == me()){
          p = findFree(u.pos);
        }
        if(p == u.pos){
          it->second.moves = false;
          //The position this unit is on is targeted now
          target(p);
        }
        else{
          //The position this unit is going towards is targeted now
          target(p);
          it->second.dir = posToDir(u.pos,p);
          it->second.prio = PRIO_CELL_ENEMY;
          it->second.canChange = false;
        }
      }
    }
    
  }
}
/*  Does a layer of BFS and updates things if needed */
void getMovementv4(int d, Pos pini, set<Pos>& vis, queue<InfoCell>& toVis, map<Pos,InfoMove>& opt){
  
  while(not toVis.empty()){
    Pos act = toVis.front().pact;
    Pos ant = toVis.front().pant;
    int dist = toVis.front().dist;
    if(dist > d){
      return;
    }
    toVis.pop();
    queueAdj(InfoCell(act,ant,dist),vis,toVis);

    if(d == 1){
      updateOptions(pini,act,opt);
    }
    else if(d == 2){
      updateOptions2(act,ant,pini,opt);
    }
  }
}
/*  Makes use of complex BFS technology and dark magic in order to assign the best 1-2 distance movement to everyone */
void giveUrgentOrders(map<int,Order>& orders){
  
  //The monstruosities
  map<int,BFSstate> ordering;
  map<int,map<Pos,InfoMove>> allOptions;

  for(int id : alive){
    Unit un = unit(id);
    Pos p = un.pos;
    //cerr << "Ordering " << id << ": " << p << " ";
    BFSstate bfs;
    bfs.vis.insert(p);
    map<Pos,InfoMove> opt;
    initMap(p,opt);
    allOptions[id] = opt;
    queueAdj(InfoCell(p,p,0),bfs.vis, bfs.toVis);

    ordering[id] = bfs;
  }

  for(int i = 1; i <= 3; ++i){
    //cerr << endl << endl << "Itera bucle externo for" << endl << endl;
    for(auto it = ordering.begin(); it != ordering.end(); ++it){
      
      Unit un = unit(it->first);
      //cerr << "managing order for " << un.id << " at " << un.pos;
      getMovementv4(i,un.pos,it->second.vis,it->second.toVis,allOptions[un.id]);
      InfoMove inf;
      if(orders[un.id].canChange){
        //cerr << " can change ";
        if(i == 1){
          if(urgent(allOptions[un.id],inf)){
            //cerr << " urgent dist 1 movement";
            Order ord;
            ord.prio = inf.prio;
            if(ord.prio == PRIO_WAITZOMBIE){
              target(un.pos);
              ord.prio = PRIO_WAITZOMBIE_LOW;
              ord.moves = false;
            }
            else{
              ord.dir = posToDir(un.pos,inf.pos);
              target(inf.pos);
            }
            ord.canChange = false;
            orders[un.id] = ord;
            inf = InfoMove();
          } 
        }
        else if(i == 2){
          if(urgent(allOptions[un.id],inf)){
            //cerr << " urgent dist 2 movement";
            Order ord;
            ord.prio = inf.prio;
            ord.canChange = false;
            if(inf.pos == un.pos){
              ord.moves = false;
            }
            else{
              ord.dir = posToDir(un.pos,inf.pos);
            }
            orders[un.id] = ord;
          }
        }
        else if(i == 3){
          Order ord;
          if(un.rounds_for_zombie != -1){ //Unit is turning into a zombie

            ord.canChange = false;
            Pos movement = nearestAlly(un.pos);
            if(movement == un.pos){
              ord.moves = false;        
            }
            else{
              ord.dir = posToDir(un.pos,movement);
              //To represent that it is a rather useless paint move
              ord.prio = PRIO_DANGER;
            }
            orders[un.id] = ord;
          }
          //cerr << " assigning adj bullshit";
          Pos mov = bestAdjacent(un.pos);
          if(mov == un.pos){
            ord.moves = false;        
          }
          else{
            ord.dir = posToDir(un.pos,mov);
            //To represent that it is a rather useless paint move
            ord.prio = PRIO_CELL_ENEMYHIGH;
          }
          orders[un.id] = ord;
        }
      }
    }
  }

  for(auto it = orders.begin(); it != orders.end(); ++it){
    Order ord = it->second;
    bool changed = false;
    if(ord.prio == PRIO_COMBAT){
      ord.prio = PRIO_COMBAT_LOW;
      changed = true;
    }
    else if(ord.prio == PRIO_WAITZOMBIE){
      ord.prio = PRIO_WAITZOMBIE_LOW;
      changed = true;
    }
    if(changed) it->second = ord;
    setBit(it->first,ordered);
  }
}

//PLAY METHOD (finally)
virtual void play () {

  alive = alive_units(me());

  if(round() == 0){
    size = num_ini_units_per_clan()*4+num_ini_zombies();
    int res = size%8;
    size /= 8;
    if(res > 0) ++size;
    waitedAnt = vector<char>(size,0);
    waited = vector<char>(size,0);
    UNITS_TOO_LITTLE = num_ini_units_per_clan()/2;
  }
  else{
    waitedAnt = waited;
    waited = vector<char>(size,0);
  }

  aliveByPlayer.clear();
  for(int i = 0; i < 4; ++i){
    //Gets the number of alive units for each player
    if(i != me()){
      aliveByPlayer[i] = static_cast<int>(alive_units(i).size());
    }
    else{
      aliveByPlayer[i] = static_cast<int>(alive.size());
    }
    
  }

  //Update and/or clear data structures
  targeted.clear();
  ordered = vector<char>(size,0);
  foodmap = vector<char>(size,0);
  priority_queue<pair<int,pair<int,Dir>>> actions;
  map<int,Order> orders;
  map<Pos,FoodSpot> foods;

  //Everything is done here
  getFoodSpots(foods);
  giveUrgentOrders(orders);
  giveFoodOrders(foods,orders);
  giveZombieOrders(orders);

  //Fills the priority queue
  for(map<int,Order>::iterator it = orders.begin(); it != orders.end(); ++it){
    if(it->second.moves)
      actions.push(make_pair(it->second.prio,make_pair(it->first,it->second.dir)));
  }

  //Empties the priority queue
  while(not actions.empty()){
    int idMove = actions.top().second.first;
    Dir dirMove = actions.top().second.second;
    actions.pop();
    //cerr << "Moving " << idMove << " to " << unit(idMove).pos+dirMove << " (" << dirMove << ")" << endl;
    move(idMove,dirMove);
  }
}

};

RegisterPlayer(PLAYER_NAME);

/* ESTRATEGIA 

1.Mirar mediante BFS por capas los casos de distancia 1 y 2 de todas las unidades.
Existen las órdenes urgentes y las no urgentes. La mayoría de cosas de distancia 1 y 2 (zombies, jugadores, comida muy cercana) son órdenes urgentes. Se escoge la opción más prioritaria.

2.Asignar a cada comida la unidad más cercana solo si es razonable ir (un enemigo mucho más cercano
hace que no lo intente nadie) y si no tiene una orden urgente ya asignada

3.Mediante BFS por capas, las unidades que aún no tengan órden buscan, en orden:
Zombies
Jugadores o cadáveres (no me acuerdo del orden)

Es configurable, pero se decidió que solo una unidad iría a por una unidad enemiga (no irán nunca 2 a la vez). No acabo de saber cuál es mejor, parecen equivalentes y depende del matchup.

4.A quien aún no tenga orden, se le asigna una orden poco importante como buscar casillas sin pintar

5.Tenemos todas las órdenes en la cola, se dan.

###################################################
Estrategias especiales o destacables (en mi opinión)
###################################################

-A distancia 2 de una unidad enemiga se intenta atacar con bajísima prioridad: la idea es que se te acerque y tú le ataques después, aprovechando el 30% de probabilidad de ganar sin batalla.

-Esto último no se hace si la unidad enemiga tiene cierto número configurable de unidades más que tú,pues entonces siempre atacará después que tú. En ese caso, se espera, haciendo que la otra unidad se te acerque. Esto también suele aprovechar que tener menos unidades suele darte más fuerza.

-Las esperas como la descrita arriba solo se hacen durante un turno. Si el siguiente turno estamos igual, la unidad se moverá.

-Solo una unidad va a por una comida dada. Solo se va a por las conseguibles.

-Hay bastantes prioridades para clasificar bien los casos

-Se espera al lado de los cadáveres para matar al zombie que generan.

-Se van a buscar zombies y cadáveres (maximizar unidades vivas siempre es bueno)

-Se van a buscar unidades enemigas (dan puntos)

-Cuando se persiguen unidades o zombies se maximiza siempre la diagonalidad para evitar cosas como esta:

  ................      ................   
  .x..............  ->  .............y.. 
  .............y..      .x.............. 
  ................      ................ 

  y en su lugar tener:

  ................      ................
  .x..............  ->  ..x..........y..
  .............y..      ................
  ................      ................

  Con lo de antes no nos acercábamos, ahora nos acercamos 2 casillas

-Se gestionan bastantes casos de zombies

-Se prefiere la comida por encima de esperar al lado de un cadáver: la idea es que si te asignan para ir a por una comida es porque es muy probable que te la lleves. Muchas veces, además, el cadáver se convierte en zombie mientras aún no te has alejado mucho y lo acabas matando porque te persigue a ti.
*/





/*
MEJORAS:

arregla el warning de unit (updateOptions parte de los zombies probablemente)

para dodgear zombies: mandar una señal a otra unidad para que se aparte y te deje pasar

Tener en cuenta quién va ganando y quién va perdiendo

Decidir las prioridades en función de los parámetros de entrada

*/
