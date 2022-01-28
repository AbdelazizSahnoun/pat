/* ********************* *
 * TP2 INF3173 H2021
 * Code permanent: SAHA01119808
 * Nom: Sahnoun
 * Prénom: Abdelaziz
 * ********************* */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signalfd.h>
/*
 * Fonction qui sert à traiter l'affichage des lignes de séparations
 * separateur : l'indicateur de séparation
 * sortie : est ce qu'il s'agit de stdout(1) ou de stderr(2)
 * numeroSortie : le numéro de la sortie à afficher
 * newline : indicateur pour dire s'il faut rajouter une '\n' et un 4eme séparateur : 0 si non 1 si oui
 * nbCommandes : le nombre de commandes que pat affiche, s'il s'agit d'une seule commande, on ignore le
 * numeroSortie
 */

char* printSortie(char* separateur,int sortie,int numeroSortie,int newline,int nbCommandes){
    char* retour=malloc(sizeof(char )*strlen(separateur)*20+1);
    if (retour == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire.");
        return NULL;
    }
    char* stringRetour=malloc(sizeof(char)*50);
    if (stringRetour == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire.");
        return NULL;
    }
    if( newline ) {
        strcat(retour,"\n");
        strcat(retour,separateur);
    }
    strcat(retour,separateur);
    strcat(retour,separateur);
    strcat(retour,separateur);

    if( sortie == 1 ){
        strcat(retour," stdout");
        if(nbCommandes>1) {
            sprintf(stringRetour, "%s %d", retour, numeroSortie);
        } else {
            strcpy(stringRetour,retour);
        }
    } else {
        strcat(retour," stderr");
        if(nbCommandes>1) {
            sprintf(stringRetour, "%s %d", retour, numeroSortie);
        } else {
            strcpy(stringRetour,retour);
        }
    }

    free(retour);
    strcat(stringRetour,"\n");
    return stringRetour;


}
/*
 * Fonction qui sert à traiter l'affichage des lignes de séparation lors des exit
 * separateur : l'indicateur de séparation
 * numeroExit : le numéro de l'exit à afficher
 * status : le status de la commande à afficher
 * signal : 0 si la commande a exit sans signal, 1 s'il y a eu un signal pour l'exit
 * numeroSignal : le signal de la commande a afficher
 * nbCommandes : le nombre de commandes que pat affiche, s'il s'agit d'une seule commande, on ignore le
 * numeroExit
 */
char* printSortieExit(char* separateur,int newline,int numeroExit, int status,int signal,int numeroSignal,int nbCommandes){
    char* retour=malloc(sizeof(char )*strlen(separateur)*20+1);
    if (retour == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire.");
        return NULL;
    }
    char* stringRetour=malloc(sizeof(char)*50);
    if (stringRetour == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire.");
        return NULL;
    }
    if( newline ) {
        strcat(retour,"\n");
        strcat(retour,separateur);
    }

    strcat(retour,separateur);
    strcat(retour,separateur);
    strcat(retour,separateur);



    if(nbCommandes>1) {
        strcat(retour," exit");
        sprintf(stringRetour,"%s %d,",retour,numeroExit);
        strcpy(retour,stringRetour);
    } else {
        strcat(retour," exit,");
        strcpy(stringRetour,retour);
    }


    if( signal == 1 ){
        strcat(retour," signal=");
        sprintf(stringRetour,"%s%d",retour,numeroSignal);
    } else {
        strcat(retour," status=");
        sprintf(stringRetour,"%s%d",retour,status);
    }

    free(retour);
    strcat(stringRetour,"\n");
    return stringRetour;


}



int main(int argc,char** argv) {


    if (argc < 2) {
        fprintf(stderr, "pat [-s séparateur] commande arguments [+ commande arguments]...\n");
        return 1;
    }
    char *separateur = "+"; // cette variable va contenir le separateur
    int nbCommandes = 0;
    int debutCommandes = 1; // sert à identifier la position dans argv qui correspond a la premiere commande
                            // argv[1] sans l'options -s sinon argv[3] avec l'option -s

    if (strcmp(argv[1], "-s") == 0) {

        //pour s'assurer que le separateur soit specifie si on rentre le -s comme option
        if (argc < 4) {
            fprintf(stderr, "pat [-s séparateur] commande arguments [+ commande arguments]...\n");
            return 1;
        } else {
            separateur = NULL;
            separateur = realloc(separateur, sizeof(char) * strlen(argv[2]) + 1);
            if (separateur == NULL) {
                fprintf(stderr, "Impossible d'allouer de la mémoire.");
                return 1;
            }
            strcpy(separateur, argv[2]);
            debutCommandes = 3;
        }


    }

    /*
     * On va avoir un tableau qui va contenir les positions dans argv des debut et fins des commandes
     * par exemple : /pat -s @1 ./pat -s @2 ./prog1 @2 ./prog2 @1 ./prog3
     * on avoir dans le tableau positions : 3,9, qui veut dire de argv[3] a argv[8],
     * on a une commande au complet soit : ./pat -s @2 ./prog1 @2 ./prog2
     * et on aura aussi positions : 9.11 qui veut dire qu'à argv[10] on a ./prog3
     */
    int positions[argc-1];
    positions[0]=debutCommandes;
    int int_positions=1;
    for (int i = debutCommandes; i < argc; i++) {

        if (strcmp(argv[i], separateur) == 0) {
            positions[int_positions]=i;
            int_positions++;
            nbCommandes++;
        }
    }
    positions[int_positions]=argc;
    nbCommandes++;



    ///////////////////////////////////////////////////////////////


    char **arguments=NULL; // variable qui va contenir une commande et un NULL à la fin pour le exec
    arguments=realloc(arguments, sizeof(*arguments)*(argc));
    if (arguments == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire.");
        return 1;
    }




    char buffer[500];
    memset(buffer, 0, sizeof(buffer));


    // On utilise signalfd qui va nous servir à capturer le signal SIGCHLD
    // et va nous permettre de faire le wait au bon moment
    // cela permet d'avoir donc un file descriptor qu'on pourra monitorer avec un poll
    // car les signal handlers interrompt le poll
    // source : https://www.linuxprogrammingblog.com/code-examples/signalfd
    int sfd;
    sigset_t mask;
    sigemptyset (&mask);
    sigaddset (&mask, SIGCHLD);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) < 0) {
        perror ("sigprocmask");
        return 1;
    }
    sfd = signalfd (-1, &mask, 0);
    if (sfd < 0) {
        perror ("signalfd");
        return 1;
    }



    // pour chaque commande, on aura 2 file descriptor, un pour le stdout et un pour le stderr
    // et finalement un dernier pour le signal fd donc 2*commandes + 1

    struct pollfd *fds = NULL;
    nfds_t nfds = nbCommandes*2+1;
    int resPoll;
    fds = malloc(sizeof(struct pollfd) * nfds);
    if (fds == NULL) {
        perror("Impossible d'allouer de la mémoire");
        return 1;
    }

    int fd_pipes_stdout[nbCommandes*2][2];
    int fd_pipes_stderr[nbCommandes*2][2];

    int returnpipe;
    for(int i=0;i<nbCommandes;i++){
        returnpipe=pipe(fd_pipes_stdout[i]);
        if(returnpipe==-1){
            perror("fd_pipes_stdout");
            return 1;
        }
        returnpipe=pipe(fd_pipes_stderr[i]);
        if(returnpipe==-1){
            perror("fd_pipes_stderr");
            return 1;
        }
    }


    //on va avoir par exemple pour la 1ere commande, 0 = fd du pipe stdout et 1 = fd du pipe stderr, etc
    //ou les fd paires correspondent au stdout et les impairs au stderr, car pour chaque commande,
    //le stdout et le stderr se suivent
    for(int i=0;i<nbCommandes*2;i=i+2){
        fds[i].fd = fd_pipes_stdout[i/2][0];
        fds[i].events = POLLIN;
        fds[i+1].fd = fd_pipes_stderr[i/2][0];
        fds[i+1].events = POLLIN;
    }

    fds[nbCommandes*2].fd = sfd;
    fds[nbCommandes*2].events = POLLIN;


    int i = 0;
    int fd_open=nbCommandes; // va contenir le nombre de fd ouvert, une fois qu'on a terminé nos operations
                             // avec ses fd, on les fermes, on decrémente pour sortir du while qui s'en vient
    pid_t  pid_fils;
    int newline=0; // pour le printSortie et printSortieExit

    pid_t  pid[nbCommandes]; // va contenir l'ensemble des resultats des pids resultant de fork
                             //// pour le wait a la fin

    int sommeRetours=0; // pour le return a la fin
    // va contenir la sortie des fonctions qui creent le separateur

    char* stringRetour=malloc(sizeof(char )*30);
    if (stringRetour == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire.");
        return 1;
    }
    int compteurDePositions=0; // compteur de l'indice du tableau positions

    int memeSeparation=-1;   // va contenir la valeur de j pour savoir si on affiche toujours la meme sortie
                             // afin d'eviter d'afficher le separateur

    int debut=0;             // variable qui indique seulement qu'il s'agit du premier affichage, donc
                             // on compare pas avec ancienbuffer

    char ancienbuffer[550];   // variable qui va contenir la sortie affiché précédemment afin de savoir
                              // s'il faut mettre un '\n' ou non
    memset(ancienbuffer, 0, sizeof(ancienbuffer));

    do {

        for (; i < nbCommandes; i++) {
            pid_fils = fork();
            pid[i]=pid_fils;

            if (pid_fils == -1) {
                perror("Echec du fork");
                return 127;
            } else if (pid_fils == 0) {


                //on parcout le tableau de positions puis on stocke les arguments de la commande actuelle
                // dans un nouveau tableau qu'on rajoute un NULL a la fin pour le exec
                int z=0;
                for(int k=positions[compteurDePositions];k<positions[compteurDePositions+1];k++){
                    if (strcmp(argv[k], separateur) != 0) {
                        *(arguments + z) = malloc(sizeof(char) * strlen(argv[k]) + 1);
                        strcpy(arguments[z], argv[k]);
                        z++;
                    }

                }

                arguments[z]= NULL;



                close(fd_pipes_stdout[i][0]);
                dup2(fd_pipes_stdout[i][1], 1);

                close(fd_pipes_stderr[i][0]);
                dup2(fd_pipes_stderr[i][1], 2);


                close(fd_pipes_stdout[i][1]);
                close(fd_pipes_stderr[i][1]);


                execvp(arguments[0], arguments);
                perror(arguments[0]);
                return 127;

            }
            arguments=realloc(arguments, sizeof(*arguments)*(argc));
            memset(buffer, 0, sizeof(arguments));
            compteurDePositions++;

        }


        resPoll = poll(fds, nfds, -1);
        if (resPoll == -1) {
            perror("poll");
            return 1;
        }
        for(int j=0;j<nbCommandes*2;j++) {

            if (fds[j].revents == POLLIN) {

                // si l'ancien buffer contenient une nouvelle ligne ou non et s'il agit du premier affichage
                // de pat ou non
                if( strchr(ancienbuffer,'\n')==NULL && debut!=0){
                    newline=1;
                } else {
                    newline=0;
                }

                debut=1;


                // pair il s'agit de stdout impair il s'agit de stderr
                if(j%2==0) {
                    strcpy(stringRetour,printSortie(separateur, 1, (j/2) + 1, newline,nbCommandes));
                } else {
                    strcpy(stringRetour,printSortie(separateur, 2, (j/2) + 1, newline,nbCommandes));
                }

                int resRead=read(fds[j].fd, buffer, sizeof(buffer));
                if(resRead==-1){
                    perror("read");
                    return 1;
                }

                // si on affiche toujours pour le meme j donc la meme sortie, il n'est pas necessaire
                // d'afficher le separateur
                if(memeSeparation != j) {
                    printf("%s", stringRetour);
                    fflush(stdout);
                }

                memeSeparation=j;
                int resWrite=write(1,buffer,sizeof(buffer));
                if( resWrite == -1){
                    perror("write");
                    return 1;
                }
                strcpy(ancienbuffer,buffer);
                memset(buffer, 0, sizeof(buffer));

            }

        }

        // il s'agit du signal fd
        if (fds[nbCommandes*2].revents & POLLIN ) {
            int status=0;
            int numeroExit=0;
            int resWait=waitpid(-1, &status, 0);
            if(resWait==-1){
                perror("waitpid");
                return 1;
            }

            // permet de savoir quel fils on a wait pour pouvoir afficher le bon numero d'exit
            for(int k=0;k<nbCommandes;k++){
                if(pid[k]==resWait)
                    numeroExit=k+1;
            }
                                                      //pour un cas d'erreur ou l'ancien buffer est null
                                                      //et un sepateur de plus est ajouté
            if( strchr(ancienbuffer,'\n')==NULL && strcmp(ancienbuffer,"")!=0){
                fflush(stdout);
                newline=1;
            } else {
                newline=0;
            }

            int resRead=read(fds[nbCommandes*2].fd, buffer, sizeof(buffer));
            if(resRead==-1){
                perror("read");
                return 1;
            }

            if(WIFEXITED(status)){

                strcpy(stringRetour,printSortieExit(separateur,newline,numeroExit,WEXITSTATUS(status),0,0,nbCommandes));
                sommeRetours+=WEXITSTATUS(status);

            }else if( WIFSIGNALED(status)) {

                strcpy(stringRetour,printSortieExit(separateur,newline,numeroExit,0,1,WTERMSIG(status),nbCommandes));
                sommeRetours+=127+WTERMSIG(status)+1;
            }

            printf("%s",stringRetour);
            fflush(stdout);
            strcpy(ancienbuffer,stringRetour);
            memset(buffer, 0, sizeof(buffer));

            fd_open--; // si on a wait, on a un fd de moins

            if(fd_open==0){
                // on ferme tout les fd ouverts
                int resClose=close(sfd);
                if(resClose==-1){
                    perror("close");
                    return 1;
                }
                for(int k=0;k<nbCommandes;k++){
                    resClose=close(fd_pipes_stdout[k][0]);
                    if(resClose==-1){
                        perror("close");
                        return 1;
                    }
                    resClose=close(fd_pipes_stderr[k][0]);
                    if(resClose==-1){
                        perror("close");
                        return 1;
                    }
                    resClose=close(fd_pipes_stdout[k][1]);
                    if(resClose==-1){
                        perror("close");
                        return 1;
                    }
                    resClose=close(fd_pipes_stderr[k][1]);
                    if(resClose==-1){
                        perror("close");
                        return 1;
                    }
                }
                for(int k=0;k<nbCommandes*2;k++){
                    fds[k].fd=-1;
                }

                fds[nbCommandes*2].fd=-1;


            }


        }




    }while (fd_open>0);


    free(arguments);
    free(stringRetour);
    free(fds);
    return sommeRetours;


}

