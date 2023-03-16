#include <iostream>
#include <vector>
#include <random>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <map>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <algorithm>
using namespace std;

/*

ESP:

-The Walking Dead Tester by DarkJaslo-

Para saber cómo utilizarlo, compílalo y ejecútalo sin parámetros para que te vaya solicitando los parámetros.

Crea ficheros del tipo tX.test temporales que se borran al final.
Si abortas el programa antes de tiempo, alguno se quedará ahí. (y me dio pereza que SIGINT los borrara)

Para el número de cores, pondría el total de CPUs lógicas (cores x hilos/core)


ENG:

-The Walking Dead Tester by DarkJaslo-

If you want to know how to use it, compile and execute it without any parameters and it will prompt you for everything.

It creates temporal files following the format tX.test (where X is a number) that are deleted when the program ends.
Were you to abort the program, or should it crash, they won't be deleted (and I'm too lazy to handle signals to do it)

Regarding the number of cores asked by the program, I'd go with your machine's total CPU threads.

PD: The rest of the program is in Spanish. Use Google Translator or something, because I didn't want to translate it.

*/

vector<int> glob_scores = {0,0,0,0};
vector<vector<int>> positions(4,vector<int>(4,0));

int winner(string filename){
    ifstream file;
    file.open(filename);

    if(file.fail()){
        cout << "Error abriendo el fichero " << filename << endl;
        exit(420);
    }

    string line;

    while(not file.eof()){
        
        getline(file,line);

        if(line == "round 200"){
            getline(file,line); //Basura
            getline(file,line);
            istringstream ss(line);
            string aux;
            ss >> aux;
            vector<int> scores(4);
            vector<int> ordscores(4);
            vector<bool> assigned(4,false);
            int imax = -1;
            int max = 0;
            for(int i = 0; i < 4; ++i){
                string score;
                cout << score;
                ss >> score;
                scores[i] = stoi(score);
                ordscores[i] = scores[i];
                if(scores[i] > max){
                    max = scores[i];
                    imax = i;
                }
                glob_scores[i] += scores[i];
            }
            sort(ordscores.begin(), ordscores.end(), greater<int>());
            
            for(int i = 3; i >= 0; --i){
                int j = 3;
                while(1){
                    if(scores[j] == ordscores[i]){
                        positions[j][i]++;
                        break;
                    }
                    --j;
                }
            }


            file.close();
            return imax;
        } 
    }
    cout << "Game crashed ";
    file.close();
    return -1;
}

struct InfoGame{
    string filename;
    int pid;
    int seed;
    InfoGame(){
        pid = -1;
        seed = -1;
    }
};

int main(){
    cout.setf(ios::fixed);
    cout.precision(2);

    cout << "Escribe cuántas pruebas quieres hacer: " << endl;
    int npruebas;
    cin >> npruebas;
    cout << "Escribe los nombres de los jugadores: p1 p2 p3 p4" << endl;
    char p1[13];
    char p2[13];
    char p3[13];
    char p4[13];
    cin >> p1 >> p2 >> p3 >> p4;
    vector<string> pl = {p1,p2,p3,p4};
    vector<int> wins(4,0);
    
    cout << "Escribe cuántos procesos quieres a la vez (depende del pc):" << endl;
    int nprocs;
    cin >> nprocs;
    if(nprocs > npruebas) nprocs = npruebas;
    vector<InfoGame> files(nprocs);

    for(int i = 0; i < files.size(); ++i){
        string s;
        s.append("t");
        s.append(to_string(i));
        s.append(".test");
        files[i].filename = s;
    }

    srand(time(NULL));
    int crashed = 0;

    int procAct = 0;
    int i = 0;
    while(i < npruebas){

        if(procAct < nprocs and i + procAct < npruebas){ //Hay que crear proceso(s)

            //Crea un proceso que cree un juego
            ++procAct;
            int seed = rand();
            string filename;
            int index;
            for(index = 0; index < files.size(); ++index){
                if(files[index].pid == -1){
                    filename = files[index].filename;
                    break;
                }
            }
            files[index].seed = seed;

            files[index].pid = fork();

            if(files[index].pid == 0){
                //Crea juego

                //Cambiar a parámetro legible por consola si quieres otra config
                int fd = open("./default.cnf",O_RDONLY);
                dup2(fd,0);
                close(fd);

                int fd2 = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                dup2(fd2,1);
                close(fd);

                close(2);   

                //int seed = rand();
                char buf[64];
                sprintf(buf,"%d",seed);
                execlp("./Game","Game",p1,p2,p3,p4,"-s",buf,NULL);
                cerr << "Error de execlp" << endl;
                exit(1);
            }
            else if(files[index].pid == -1){
                cerr << "Error de fork" << endl;
                exit(1);
            }
        }
        else{ //No se pueden crear más procesos

            int st;
            int pid = waitpid(-1,&st,0);
            if(WIFEXITED(st)){
                string filename;
                int which = -1;
                for(int in = 0; in < files.size(); ++in){
                    if(files[in].pid == pid){
                        filename = files[in].filename;
                        which = in;
                        break;
                    }
                }
                int w = winner(filename);
                if(w >= 0 and w <= 3){
                    ++wins[w];
                    cout << "G" << i << ", SEED: " << files[which].seed << /*" Winner"*/": " << pl[w] << " (" << w << ")" << endl;
                }
                else {
                    cout << "SEED: " << files[which].seed << " CRASHED" << endl;
                    ++crashed;
                }
                ++i;
                --procAct;
            }
            
            for(int in = 0; in < files.size(); ++in){
                if(files[in].pid == pid){
                    files[in].pid = -1;
                }
            }
        } 
    }

    while(waitpid(-1,NULL,0) > 0);

    for(int i = 0; i < files.size(); ++i){
        remove(files[i].filename.c_str());
    }
    int sum = 0;
    for(int i =0; i < wins.size(); ++i){
        sum += wins[i];
    }


    cout << endl << "#################" << endl;
    for(int i = 0; i < wins.size(); ++i){
        
        cout << "Wins " << pl[i] << ": " << wins[i] << " (" << (double(wins[i])/sum)*100 << "%)    Avg score: " <<
        double(glob_scores[i]/npruebas) << " ";
        cout << "1st: " << positions[i][0] << " (" << double(positions[i][0])/npruebas*100 << "%) ";
        cout << "2nd: " << positions[i][1] << " (" << double(positions[i][1])/npruebas*100 << "%) ";
        cout << "3rd: " << positions[i][2] << " (" << double(positions[i][2])/npruebas*100 << "%) ";
        cout << "4th: " << positions[i][3] << " (" << double(positions[i][3])/npruebas*100 << "%)" << endl; 
    }
    cout << "Times crashed: " << crashed << endl;
    cout << "#################" << endl;  
}
