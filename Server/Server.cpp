#include <dirent.h>
#include <bits/stdc++.h>
#include <czmq.h> 
using namespace std; 


//typedef unordered_map<string,vector<string> > SongList; // podrá se la llave char * ? para guardar tcp... o el id se lo ingreso por consola?

typedef vector<string> S;
S SongList,AddList;

// Se tendrá un diccionario que indique qué canción ha sido reproducida y la cantidad de veces , también un contador global de las
// Canciones por servidor.

typedef unordered_map<string,int> SR; // Lista cada canción con sus reproducciónes.
SR SongRate;
int GlobalCounter=0; // Contador global para el manejo del número de reproducciones de un servidor.
int HitsNumb=2; // Tope de las canciones para que sean un hit e inicien difusión.

void SendList(zmsg_t* msg , void * Brokers){

    for(int i=0; i<SongList.size();i++){
        zmsg_addstr(msg, SongList[i].c_str());
      }

    zmsg_send(&msg, Brokers); // Enviamos el mensaje de registro junto a toda la lista de canciones del servidor
    // [ServerReg|DirServer|lista de canciones.................]

}


void RegServer(void* Brokers, string DirServer){
  /*
      Enviamos mensaje para que el broker sepa de nuestra existencia junto a la lista actual.
  */
  zmsg_t* regmsg = zmsg_new();
  zmsg_addstr(regmsg, "ServerReg");
  zmsg_addstr(regmsg, DirServer.c_str());
  SendList(regmsg ,Brokers); // Añadimos todas las canciones que se cargaron al vector SongList.
}


int SendFile(void* Clients,string fname,string path1 , string path2 ){
  /*
    Se enviará el archivo al cliente 
  */

  cout << "Requested fname: " << fname << endl;

  zfile_t *file = zfile_new(path1.c_str(),fname.c_str());
  //cout << "File is readable? " << zfile_digest(file) << endl;
  zfile_close(file);  
  zchunk_t *chunk = zchunk_slurp(path2.c_str(),0);
  if(!chunk) {
    cout << "Cannot read file!" << endl;
    return 1;
  }
  cout << "Chunk size: " << zchunk_size(chunk) << endl;

  zframe_t *frame = zframe_new(zchunk_data(chunk), zchunk_size(chunk));
  zframe_send(&frame, Clients, 0);
 
  return 0;

}

void RegLastSong(string Song){ // Difusión : solo actualizamos la canción que fue actualizada
    SongList.push_back(Song); // Lista de canciones
    SongRate[Song]=0; // Añadimos a SongRate

}

void QueryFile(zctx_t* context,zmsg_t* answer){ 
      
      char * DirServerSong = zmsg_popstr(answer); // Para que se conecte con alguno que sí la tenga
      cout<<DirServerSong<<endl;
      char * Song = zmsg_popstr(answer);
      cout<<Song<<endl;
      RegLastSong(Song); // !!!!!!! Registramos la canción nueva para que se difunda a todos los clientes   
      void* server = zsocket_new(context,ZMQ_REQ);
      zsocket_connect(server,DirServerSong);     
      zstr_send(server, Song); // Canción a replicar
     
      zfile_t *download = zfile_new("./Music",Song); // Music
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
      zsocket_disconnect(server,DirServerSong); 

}



int DirSongs(string path,int op){ // 0 = Canciones 1 - Publicidad 
  /*
      - Lista y Envía la lista de reproducción cada vez que sea invocada [por primera vez o por actualización por tiempo]
        Debe listar tanto para el Vector de canciones como para el mapa 
  */
  //SongList.clear();
  int i=0,index=0;
  DIR *dir;
  struct dirent *ent;
    if ((dir = opendir (path.c_str())) != NULL) {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
        string Song_Name = ent->d_name;
        cout<<Song_Name<<endl;
        if(Song_Name.compare(".")!=0 && Song_Name.compare("..")!=0){
            if(op==0){
            SongList.push_back(Song_Name);          
            SongRate[Song_Name]=0;
            }else if(op==1){AddList.push_back(Song_Name);}          
            
                       
            }         
        }
            closedir (dir);
       
          } else {
            /* could not open directory */  
            perror ("");
            return EXIT_FAILURE;
        }

        cout<<"Canciones y Publicidades Listadas!...."<<endl;            
}

void ActualServer(void* Brokers, string  DirServer, char * Song){
  /*
      Enviamos mensaje para que el broker sepa de nuestra existencia junto a la lista actual.
  */
  zmsg_t* regmsg = zmsg_new();
  zmsg_addstr(regmsg, "ActualServer");
  zmsg_addstr(regmsg, DirServer.c_str());
  zmsg_addstr(regmsg, Song);
  zmsg_send(&regmsg,Brokers);
}

void handleBrokerMessage(zmsg_t* msg,void* Brokers,zctx_t* context,string LocalDir){
  /* 
      
      - Inicio de distribución de hits dado un umbral : los 
        servidores retornarán cada hit con su número de reproducciones, si 
        pasan el umbral se replicarán (si es posible) en el servidor con menos
        reproducciones globales.

        formato devuelto por el servidor : [Dirección_Servidor[id_servidor]|Tema|Reproducciones|Rep_globales_servidor]
        Si se retorna un mensaje por parte del broker con el código win , se procederá a enviar el archivo a
        el servidor con menos reproducciones : el posible formato devuelto es: 
         [ID_Servidor_origen|Dirección_Servidor_enviar[id_servidor]|Tema|]

        Nota : el broker se encargará de seleccionar los servidores que tengan menos reproducciones y no contengan la canción.
 
 */

  cout << "Handling the following Broker" << endl;
  zmsg_print(msg);
  //zframe_t* id = zmsg_pop(msg); // id Broker 
  char* opcode = zmsg_popstr(msg);
  if (strcmp(opcode, "statics") == 0) {    
    //Enviamos al broker nuestro GlobalCounter junto a la cancion para tratarla posteriormente
    string Song = zmsg_popstr(msg);
    zmsg_t* answer=zmsg_new();
    zmsg_addstr(answer,"ChooseMe");    
    zmsg_addstr(answer,to_string(GlobalCounter).c_str());
    zmsg_addstr(answer,Song.c_str());
    zmsg_print(answer);
    zmsg_send(&answer,Brokers);

    
  } else if (strcmp(opcode, "win") == 0) {   
    cout<<"WIN!! "<<endl;
    zmsg_t* nmsg=zmsg_dup(msg);
    QueryFile(context,msg); // Pedimos canción para duplucar. + ActualServer    
    char * ConnectServer = zmsg_popstr(nmsg);
    char * Song = zmsg_popstr(nmsg);
    ActualServer(Brokers,LocalDir,Song); // Actualizamos canción para todos 

  } else {
    cout << "Unhandled message" << endl;
  }
  cout << "End of handling" << endl;
  free(opcode);
  zmsg_destroy(&msg);
  
}

void handleClientMessage(string song,void* Clients,void* Brokers){
   /* 
      - Solicitud de conexión para reproducción : 
        Se recibirá por parte de los clientes un mensaje con el posible formato :
        [id_cliente|nombre_canción]
        Luego se enviará la canción con el siguiente posible formato [id_cliente|canción]
        - Se debe aumentar en el mapa el número de reproducciones y el global.       
  */
    GlobalCounter++;

    cout << "Handling the following CLient" << endl;      
    cerr<<"------>"<<song<<endl;
     if(song.compare("Adds")==0){ // si es un Add
        string path1="./Adds/";
        int index = rand() % AddList.size(); 
        string SelectAdd = AddList[index];
        string path2=path1+SelectAdd;
        SendFile(Clients,SelectAdd,path1,path2);
     }else{
         
         int aux=SongRate[song];
         SongRate[song]=aux+1; // Aumentamos uno a la cantidad de reproducciones de la canción
         cout<<"Hola argv"<<SongRate[song]<<endl;
         SendFile(Clients,song,"./Music/","./Music/"+song); // si es una canción
      //if(SongRate[song]%HitsNumb==0){
        if(SongRate[song]%HitsNumb==0){
        zmsg_t* msg= zmsg_new();      
        zmsg_addstr(msg,"Hit");
        zmsg_addstr(msg,song.c_str());
        zmsg_send(&msg,Brokers);      
      }
      
     }

  


}





int main(int argc, char** argv){

   if (argc != 5) {
    cerr << "Wrong call\n";
    return 1;
  }

  DirSongs("./Music",0); // SongsList Dir
  DirSongs("./Adds",1); // AddList Dir 

  string DirServer = argv[1];  
  string DirBroker = argv[2];  
  string BrokerPort = argv[3];

  string BrokerSite="tcp://"+DirBroker+":"+BrokerPort;
  string ClientPort(argv[4]);
  string ClientSite="tcp://*:"+ClientPort;
  
  string LocalDir = "tcp://"+DirServer+":"+ClientPort;

  zctx_t* context = zctx_new();
  void* Brokers = zsocket_new(context, ZMQ_DEALER);
  int BrPort = zsocket_connect(Brokers, BrokerSite.c_str());
  cout<<"Broker dir: "<<BrokerSite<<endl;
  cout << "connecting to broker: " << (BrPort == 0 ? "OK" : "ERROR") << endl;


  void* Clients = zsocket_new(context, ZMQ_REP);
  int ClPort = zsocket_bind(Clients, ClientSite.c_str());
  cout << "Listen to Clients at: "
       << ClientSite<< endl;

  zmq_pollitem_t items[] = {{Brokers, 0, ZMQ_POLLIN, 0},
                            {Clients, 0, ZMQ_POLLIN, 0}};

  cout << "Listening!" << endl;      
  // Enviamos Lista de reproducción existente
  RegServer(Brokers,LocalDir);

  while (true) {
    zmq_poll(items, 2, 10 * ZMQ_POLL_MSEC);
    if (items[0].revents & ZMQ_POLLIN) {
      cerr << "From workers\n";
      zmsg_t* msg = zmsg_recv(Brokers);
      handleBrokerMessage(msg,Brokers,context,LocalDir);
    }
    if (items[1].revents & ZMQ_POLLIN) {
      cerr << "From clients\n";
      string fname(zstr_recv(Clients));
      handleClientMessage(fname,Clients,Brokers);
    }
  }

  zctx_destroy(&context);
	return 0;
}

//  ./Server Dirserver Dirbroker brport clientport
// ./Server localhost localhost 5555 6666 
