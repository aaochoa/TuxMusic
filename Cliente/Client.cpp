#include <bits/stdc++.h>
#include <czmq.h> 
#include <SFML/Audio.hpp>



using namespace std; 

const char * Tpath;
mutex save;
int  Index=0,comm=0;

/* 
    -El cliente enviará al comenzar su primera ejecución una petición para conocer la lista de reproducción general.
    -La lista de reproducción se guardará según un usuario y se hará de manera local(por ahora sin criptar) --> pensar en el sistema
    -Deberá tener hilos para que; mientras escuche una canción pueda buscar otra o accer de a un menú de opciones.

*/
 
typedef vector<string> Pl; // PlayList.
Pl Playlist;
typedef unordered_map<string,vector<char *>> M; // para la conexión remota , aquí se cargará el mensaje con la lista global.
M MasterList;
typedef vector<string> A ; // Guardamos lo
A ListAux;

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

sf::Music music; // Para reproducir
sf::Time t1 ;


// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void RegClient(void* Brokers){
  /*
      Enviamos mensaje para que el broker sepa de nuestra existencia.
  */
  zmsg_t* regmsg = zmsg_new();
  zmsg_addstr(regmsg, "ClientReg");
  zmsg_send(&regmsg, Brokers);
  zmsg_destroy(&regmsg); 
}

void BuildMasterList(zmsg_t* msg){
  string aux; // para armar el vector de solo canciones.
  int size=zmsg_size(msg)/2; //Debido a que es un par  [Song|Dir]
  MasterList.clear();
  ListAux.clear();
  while(size>0){
    string song(zmsg_popstr(msg));   
    ListAux.push_back(song); // para búsqueda fácil
    char* DirServidor = zmsg_popstr(msg);
    MasterList[song].push_back(DirServidor); // si hay un servidor con la misma canción añade más servidores a una sola canción 
    size--;    
    } 

  zmsg_destroy(&msg);
  cout<<"::::::::::::::::::::::::::::::"<<endl;  
  cout<<"Mostrando  lista de canciones"<<endl;
     for ( auto it = MasterList.begin(); it != MasterList.end(); it++ ){
          cout << " Cancion: " << it->first<<endl;       
        for (auto i :it->second){
          cout<<"servidor(es): "<<i<<endl;
        }
    }

}



void handleServerMessage(zmsg_t* msg){
  /* 
    - Manejamos el archivo enviado por el servidor 
      Especificar zchunk. 
  */
  cout << "Handling the following Server" << endl;
  //zmsg_print(msg);
  // Retrieve the identity and the operation code 

}



void QueryFile(zctx_t* context,string SelectSong,int op){ 
      
      int VecSize=MasterList[SelectSong.c_str()].size();
      cout<<"vector size: "<<VecSize<<endl;
      int index=rand() % VecSize  ; // ???????????????????????????????????????????
      cout<<"Index: "<<index<<endl;
      char * ServerDir = MasterList[SelectSong.c_str()][index]; // ????????????????????????????????????????????????? 
      cout<<"Server ID: "<<ServerDir<<endl;
      void* server = zsocket_new(context,ZMQ_REQ);
      int o = zsocket_connect(server,ServerDir);
      //cout <<"-->>>>>>>>>>>>>>>>>>>>>>>>> "<<o<<endl;
      remove("./Temp/Temp_data.ogg");
      if(op==0){zstr_send(server, "Adds");}// Comercial
      if(op==1){zstr_send(server, SelectSong.c_str());} // Canción
     
      zfile_t *download = zfile_new("./Temp", "Temp_data.ogg");
      zfile_output(download);

      int i = 0;
      while(true){
        zframe_t *filePart = zframe_recv(server);
        cout << "Recv " << i << " of size " << zframe_size(filePart) << endl;
        zchunk_t *chunk = zchunk_new(zframe_data(filePart), zframe_size(filePart)); 
        zfile_write(download, chunk, 0);
        if(!zframe_more(filePart)) break;
        i++;
      }
      zfile_close(download);
      cout << "Complete " << zfile_digest(download) << endl;
      zsocket_disconnect(server,ServerDir); 

}



void handleBrokerMessage(zmsg_t* msg){
 /*
    - Recibe Lista de reproducción global: 
      [Song|DirServer...........] 
  */

  cout << "Handling the following Broker" << endl;
  zmsg_print(msg);
  char* opcode = zmsg_popstr(msg);
  if (strcmp(opcode, "Master") == 0) {    
  // Se esperará a la lista de reproducción para cargarla a memoria.
     BuildMasterList(zmsg_dup(msg));

  }else{
    cout << "Unhandled message" << endl;
  }
  cout << "End of handling" << endl;
  free(opcode);
  zmsg_destroy(&msg);
 
}   


void PollItems(void * Broker ,zmq_pollitem_t items[]){


  while(true){
    zmq_poll(items, 1, 10 * ZMQ_POLL_MSEC);
    if(items[0].revents & ZMQ_POLLIN){
      cerr << "From Brokers\n";
      zmsg_t* msg = zmsg_recv(Broker);
      handleBrokerMessage(msg);
    }
  }


}


void RegularPlay(zctx_t* context , string thing){
int stop=0;
   if(rand()% 3 == 2){ // Publicidad, se espera hasta que termine.
      QueryFile(context,thing,0);
      cout<<"Pauta publicitaria!!!"<<endl; 
      if(music.openFromFile(Tpath))
          music.stop();
          music.play();
          while(stop==0){
            if(music.getStatus()==0)
              {stop=1;}                   

      }                          
   }      
   cout<<"Reproduciendo : "<<thing<<endl; 
   QueryFile(context,thing,1); 
   if(music.openFromFile(Tpath))
      music.stop();
      music.play();                  
}


void PlayerList(zctx_t* context){            
  while(true){
    save.lock();
    int fin = Playlist.size();
    int PlayerStatus = music.getStatus();
    int otro = comm;
    int ind=Index;
    int ini=0;
    save.unlock();
      if(PlayerStatus==0 && otro == 0 && ind != fin && ini != fin ){          
         RegularPlay(context,Playlist[ind]);
         save.lock(); Index++; save.unlock();         
       }

    }
}

int main(int argc, char** argv){

     if (argc != 4) {
    cerr << "Wrong call\n";
    return 1;
  }
  

  string BrokerDir=argv[1];
  string BrokerPort=argv[2];

  string BrokerConnect="tcp://"+BrokerDir+":"+BrokerPort; // primera ejecución se conectará con el broker
  Tpath = argv[3];
  cout<<"Path"<<Tpath<<endl;

  zctx_t* context = zctx_new();

  void* Broker = zsocket_new(context, ZMQ_DEALER);
  int a = zsocket_connect(Broker, BrokerConnect.c_str());
  cout << "connecting to broker: "<<BrokerConnect << (a == 0 ? " OK" : "ERROR") << endl;
  
  zmq_pollitem_t items[] = {{Broker, 0, ZMQ_POLLIN, 0}};
                            //,{Servers, 0, ZMQ_POLLIN, 0} };

  cout << "Listening!" << endl;  
  RegClient(Broker);



  thread Poll(PollItems,Broker,items);
  Poll.detach();
  thread Player(PlayerList,context);
  Player.detach();


  string Op = " ";
  
    while(Op!="Salir"){
    
    cout<<"╔════╗────╔═╗╔═╗"<<endl;
    cout<<"║╔╗╔╗║────║║╚╝║║"<<endl;
    cout<<"╚╝║║╠╣╔╦╗╔╣╔╗╔╗╠╗╔╦══╦╦══╗"<<endl;
    cout<<"──║║║║║╠╬╬╣║║║║║║║║══╬╣╔═╝"<<endl;
    cout<<"──║║║╚╝╠╬╬╣║║║║║╚╝╠══║║╚═╗"<<endl;
    cout<<"──╚╝╚══╩╝╚╩╝╚╝╚╩══╩══╩╩══╝"<<endl;
    cout<<"::::::::::::::::::::::::::::::::::::::::"<<endl;
    cout<<"::::::::::::::::::::::::::::::::::::::::"<<endl;
    cout<<"1) B  Parámetro --> Busca según el parámetro "<<endl;
    cout<<"2) R Nombre de la cancion --> Reproduce "<<endl;
    cout<<"3) A Nombre de la cancion ---> Añade a Playlist "<<endl;
    cout<<"4) V Playlist  ---> Muestra la Playlist "<<endl;
    cout<<"5) S salir  ----> Salir"<<endl;
    cout<<"6) Play song   ----> Play"<<endl;
    cout<<"7) Pause song   ----> Pause"<<endl;
    cout<<"8) Stop song   ----> Stop"<<endl;
    cout<<"9) Next song   ----> Next"<<endl;
    cout<<"10) Prev song   ----> Prev"<<endl;
    cout<<"::::::::::::::::::::::::::::::::::::::::"<<endl;
    cout<<"::::::::::::::::::::::::::::::::::::::::"<<endl;
    int one=0;
    string thing;
    cin>>Op>>thing;
    getchar();
    

     if(Op=="B"){            
             for (int i = 0; i < ListAux.size();i++){
              if(ListAux[i].find(thing)!=string::npos){
                if(i==0){cout<<"Posibles canciones con su parámetro de búsqueda: "<<endl;}
                    cout<<"["<<i<<"] "<<ListAux[i]<<endl;                        
                    }     
                  }
         }

       if(Op=="R"){
              comm=1;
              RegularPlay(context,thing);           
            }

        if(Op=="A"){
          
          save.lock();
          Playlist.push_back(thing);
          comm=1;
          save.unlock();
          //Playlist.push_back(thing);
          
          cout<<"La canción: "<<thing<<" Se ha añadido exitosamente "<<endl;             
        }

        if(Op=="V"){

          cout<<":::::Mostrando PlayList:::::"<<endl; 
          for (int i = 0; i < Playlist.size(); i++){
               cout<<"["<<i<<"] "<<Playlist[i]<<endl;
            }

            if(Playlist.size()>0){  
            cout<<"Desea reproducir esta lista ? Y/N"<<endl;
            string think;
            cin>>think;

            if(think=="Y"){
              Index=0;
              music.stop();
              comm=0; // Activador              
            }else{cout<<"Saliendo..."<<endl;}
  
            }
        }


        if(Op=="S"){ 
            break;
        } 

        if(Op=="Stop"){
            comm=1; 
            music.stop();
        }

        if(Op=="Pause"){ 
            music.pause();
        }

         if(Op=="Play"){ 
            music.play();
            comm=0;
        }

        if(Op=="Next"){
          
            if(Index < Playlist.size()){
              //save.lock();
              //Index++;
              //save.unlock();
              comm=0;// activa hilo
              music.stop();
              
              
            }else{cout<<"No hay más canciones para adelantar"<<endl;} 
            
        }


        if(Op=="Prev"){
            if(Index > 1){              
              save.lock();
              Index=Index-2;
              save.unlock();
              music.stop();
              comm=0;              

            }else{cout<<"No hay más canciones para Atrasar"<<endl;} 
            
        }        
    } // fin while    
  music.stop();  
  system("exec rm -rf Temp/Temp_data.ogg");   
  Poll.~thread();
  Player.~thread();
 
  zctx_destroy(&context);
  return 0;
 
}
//./Client localhost 4444 Temp/Temp_data.mp3
// http://www.ambiera.com/irrklang/features.html
// http://stackoverflow.com/questions/7180920/bass-play-a-stream
//  sudo apt-get install libsfml-dev sfml :D
// http://sfml-dev.org/tutorials/2.0/audio-sounds.php
// g++ -c 
// github//pin3da
