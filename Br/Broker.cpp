#include <bits/stdc++.h>
#include <czmq.h> 
using namespace std; 

/* 
    - Se usará un diccionario con el "id" del servidor como llave 
      y un vector de string para almacenar sus canciones.

      Nota: Un servidor "no" tiene canciones repetidas bajo un mismo nombre
      pero 2 servidores si las podrían tener, por esto se decide usar el id
      del servidor inrepetible a el nombre de la canción ya que a la hora de
      replicar un tema se duplica el lugar en donde se encuentran las canciones. 
*/


typedef unordered_map<string,vector<char *> > M; // Contenedor de todas las canciones en sus respectivos servidores.
M MasterList;
typedef vector<zframe_t*>S; // se deben comparar z_frames
S ClientList; // Para guardar la lista de servidores y clientes
typedef unordered_map<char * , zframe_t* >R; // 192.168.0.1 343532652362AF
R ServerList;
typedef unordered_map<zframe_t* , int>G; // Guardamos reproducciones globales
G ServerCounter;
int FIX1=0,FIX=0;

void RegServer(zframe_t* id,char * DirServidor){
 /*
     Registramos servidores para su posterior manejo y difusión.
  */
  zframe_print(id, "Id to add");
  zframe_t* dup = zframe_dup(id);
  zframe_print(dup, "Id copy add");
  ServerList[DirServidor]=dup;
}

void RegClient(zframe_t* id){
 /*
     Registramos Clientes para reenviar listas de reproducción actualizadas.
  */
  cout<<":::RegCLient::::"<<endl;
  zframe_print(zframe_dup(id), "Id to add");
  zframe_t* dup = zframe_dup(id);
  zframe_print(dup, "Id copy add");
  ClientList.push_back(dup);
}

void PrintMasterList(){
    for ( auto it = MasterList.begin(); it != MasterList.end(); it++ ){
    cout << " Cancion: " << it->first<<endl;
        for (auto i :it->second){
          cout<<"servidor(es): "<<i<<endl;
        }
    }
}

void PrintServerList(){
    for ( auto it = ServerList.begin(); it != ServerList.end(); it++ ){
          cout << " servidor " << it->first<<endl;       
          //zframe_print(it->second,"ID:")<<endl;
        
    }
}


void Broadcast(zmsg_t* msg ,void* Clients){      

  for(zframe_t *it : ClientList){    
      zmsg_t* msg2 = zmsg_dup(msg);      
      zframe_t* id2 = zframe_dup(it);     
      zmsg_prepend(msg2,&id2);
      zmsg_print(msg2);
      zmsg_send(&msg2,Clients);
      zmsg_destroy(&msg2);

  }    
 
}

void Unicast(zmsg_t* msg ,void* Clients,zframe_t * ClientId){
      zmsg_t* msg2 = zmsg_dup(msg);
      zmsg_prepend(msg2,&ClientId);
      zmsg_print(msg2);
      zmsg_send(&msg2,Clients);
      zmsg_destroy(&msg);
}

void SendListAll(void * Clients){ // difundimos lista a todos dado que un servidor se sume al esquema 

   zmsg_t * msg = zmsg_new();
   zmsg_addstr(msg,"Master");

   for (auto it = MasterList.begin(); it != MasterList.end(); it++){         
        for (auto i :it->second){
             
             zmsg_addstr(msg,it->first.c_str());
             zmsg_addstr(msg,i);
            }        
    }  
      Broadcast(msg,Clients); // difundimos lista de canciones a todos los clientes.
}



void SendListOne(void * Clients,zframe_t * ClientId){ // enviamos solo al que se conecte ya que no se cambia el estado del MasterList

   zmsg_t * msg = zmsg_new();
   zmsg_addstr(msg,"Master");

   for (auto it = MasterList.begin(); it != MasterList.end(); it++){         
        for (auto i :it->second){
             
             zmsg_addstr(msg,it->first.c_str());
             zmsg_addstr(msg,i);
            }        
    }
    
    Unicast(msg,Clients,ClientId); // difundimos lista de canciones al cliente 
}




void ListAllSongs(zmsg_t* incmsg,char * DirServidor){
  /*
      - Lista y guarda en MasterList todas las canciones de cada servidor con su respectiva dirección en MasterList.
  */
    cout<<"List All songs"<<endl;
    zmsg_print(incmsg);
    int size=zmsg_size(incmsg); // Tamaño del mensaje en frames (número de canciones)

    while(size>0){
    string song(zmsg_popstr(incmsg));
    MasterList[song].push_back(DirServidor); // si hay un servidor con la misma canción añade más servidores a una sola canción 
    size--;    
    }     

 PrintMasterList();
}

void HitVote(void * Servers,string Song,zframe_t* id){ // enviar mensaje a los servidores diferentes a los que tengan esa canción
      int a=0,b=0;
      for (auto it = ServerList.begin(); it != ServerList.end(); it++){
            char * aux = it->first;
            
        for (int i = 0; i < MasterList[Song].size(); i++){
              if(strcmp(MasterList[Song][i] ,aux) == 0){
                 a=1; // si está por lo menos 1 vez
                
               }
                
            } 
              if(a==0){
                FIX++;
                cout<<"!!!!!!!!!!!!!!Aux: "<<aux<<endl;
                b=1;                
                zmsg_t* msg = zmsg_new();
                zframe_t * id = zframe_dup(ServerList[aux]);
                zmsg_append(msg,&id);
                zmsg_addstr(msg,"statics");
                zmsg_addstr(msg,Song.c_str());
                zmsg_send(&msg,Servers);
                }
               a=0;    // volvemos a cero
            
          }

          if(b==0){
            cout<<"El hit "<<Song<<" se encuentra en todos los servidores , imposible replicar"<<endl;
          }else{cout<<"Eligiendo Servidor para replicar: "<<Song<<endl;}

}

void ServerChoice(void* Servers, zframe_t* id,zmsg_t* msg){
     int Repro =atoi(zmsg_popstr(msg));
     string Song =zmsg_popstr(msg);
     ServerCounter[id]=Repro; // Registramos reproducciones globales cada vez que se necesite establecer ganador
     cout<<"Hola 1"<<endl;
     if(FIX1==FIX){ // será igual hasta que el último de los servidores se reporte, se enviará el ganador si cumple
        int win=ServerCounter[id]; // -1 debido a que no se envía al que lanza el hit
        zframe_t* id2=zframe_dup(id) ;
        for (auto it = ServerCounter.begin(); it != ServerCounter.end(); it++){
             int aux=it->second;
             cout<<"Hola 2 :aux "<<aux<<endl;
             if(win>aux){ 
                id2=zframe_dup(it->first);
                win=aux;
              }
        }
        int index = rand () % MasterList[Song].size();
        char * DirServerSong=MasterList[Song][index]; // escogemos servidor que contenga la canción 
        zmsg_t* answer = zmsg_new();        
        zmsg_append(answer,&id2); // enviamos al que no la tenga para que la pida
        zmsg_addstr(answer,"win");
        zmsg_addstr(answer,DirServerSong); // Para que se conecte con alguno que sí la tenga
        zmsg_addstr(answer,Song.c_str());
        zmsg_print(answer);
        zmsg_send(&answer,Servers);
        zframe_destroy(&id2);
        FIX1=0;
        FIX=0;
     }else{cout<<"esperando a que todos los servidores se reporten...calculando ganador "<<endl;
           
      }

    
}


void handleServerMessage(zmsg_t* msg,void* Clients,void * Servers){
  /* 
      - Registro de temas:
        [Tema|Dirección_Servidor...se repite]
      
      - Inicio de distribución de hits dado un periodo de tiempo :
        formato devuelto por el servidor : [Tema|Dirección_Servidor[id_servidor]|Reproducciones|Rep_globales_servidor]
        Se retorna un mensaje win al servidor elegido, formato:  
        [win|ID_Servidor_origen|Dirección_Servidor_enviar[id_servidor]|Tema|]

        Nota : el broker se encargará de seleccionar los servidores que tengan menos reproducciones y que no contengan la canción.

      - Registrar los servidores para tener una base de datos para poder replicar canciones sin redundancias.
 
  */
  cout << "Handling the following Server" << endl;
  zmsg_print(msg);
  // Retrieve the identity and the operation code
  zframe_t* id = zmsg_pop(msg); // id Server
  char* opcode = zmsg_popstr(msg);
  if (strcmp(opcode, "ServerReg") == 0) { // Registramos servidores
      char* DirServidor = zmsg_popstr(msg);    
      RegServer(id,DirServidor); 
      // el "segundo" frame guardará la dir del servidor (el frame luego de opcode)
      ListAllSongs(msg,DirServidor);
      // Luego enviamos a los clientes registrados en ClientList
      PrintServerList();

  if(ClientList.size()>0){
      cout<<"Client list size: "<<ClientList.size()<<endl;
      SendListAll(Clients);  
    }else{cout<<"No hay clientes para enviar lista"<<endl;}
    
  } else if (strcmp(opcode, "Hit") == 0) { 
    // Si un servidor envía un hit enviamos a cada servidor registrado un mensaje para que devuelvan su GlobalCounter
   
    char * Song = zmsg_popstr(msg);
    HitVote(Servers,Song,zframe_dup(id));
    //zmsg_send(&msg,clients);
    //zmsg_send(&msg, clients);
  }else if (strcmp(opcode, "ChooseMe") == 0) {
    // El servidor con menor GC se escogera para que realize la conexión con el que tengan el hit
    FIX1++; // Aumentamos para determinar si todos respondieron para iniciar la elección (todos deben estar presentes)
    ServerChoice(Servers, id, msg);
    
  } else if (strcmp(opcode, "ActualServer") == 0) {
    // Actualizamos la lista solo con la cancion que se duplicó
    char * NewServer = zmsg_popstr(msg);
    char * Song = zmsg_popstr(msg);
    MasterList[Song].push_back(NewServer);
    cout<<"EL NEW Server"<<NewServer<<endl;
    SendListAll(Clients);  

  } else {
    cout << "Unhandled message" << endl;
  }
  cout << "End of handling" << endl;
  free(opcode);
  zframe_destroy(&id);
  zmsg_destroy(&msg);

}

void handleClientMessage(zmsg_t* msg,void* Brokers,void* Clients){
 /*
    - Lista de reproducción global: 
        Se reenviarán a los clientes la lista de reproducción entregada por los servidores[solo 1 vez].
        zmsg [id_cliente|@Nombre_de_la_cancion *servido(es) ..... ]
  */
  cout << "Handling the following Client" << endl;
  zmsg_print(msg);
  zmsg_t * incmsg=zmsg_dup(msg);

  
  zframe_t* id = zmsg_pop(incmsg); // id Client
  char* opcode = zmsg_popstr(incmsg);
  /*
  zmsg_append(hola,&id);      
  zmsg_addstr(hola,"hola mundo");
  zmsg_send(&hola,Clients);
  */

  if (strcmp(opcode, "ClientReg") == 0) { // Registramos clientes   
      RegClient(id);
      // luego enviamos la lista actual de la Master List al cliente 
      if(MasterList.size()>0){
      SendListOne(Clients,zframe_dup(id));    
      }else{cout<<" No hay servidores conectados  "<<endl;}  
  }else {
    cout << "Unhandled message" << endl;
  }
    cout << "End of handling" << endl;

  zframe_destroy(&id);
  zmsg_destroy(&msg);

}   



int main(int argc, char** argv){

   if (argc != 3) {
    cerr << "Wrong call\n";
    cout<<argc<<endl;
    return 1;
  }

  string port1 =argv[1];  
  string port2 =argv[2];

  string ServerSite="tcp://*:"+ port1;
  string ClientSite="tcp://*:"+ port2;

  zctx_t* context = zctx_new();
  void* Servers = zsocket_new(context, ZMQ_ROUTER);
  int SrPort = zsocket_bind(Servers, ServerSite.c_str());
  cout << "Listen to Servers at : "
       << "localhost:" << SrPort << endl;


  void* Clients = zsocket_new(context, ZMQ_ROUTER);
  int ClPort = zsocket_bind(Clients, ClientSite.c_str());
  cout << "Listen to Clients at: "
       << "localhost:" << ClPort << endl;

  zmq_pollitem_t items[] = {{Servers, 0, ZMQ_POLLIN, 0},
                            {Clients, 0, ZMQ_POLLIN, 0}};

  cout << "Listening!" << endl;


  while (true) {
    zmq_poll(items, 2, 10 * ZMQ_POLL_MSEC);
    if (items[0].revents & ZMQ_POLLIN) {
      cerr << "From Servers\n";
      zmsg_t* msg = zmsg_recv(Servers);
      handleServerMessage(msg,Clients,Servers);

    }
    if (items[1].revents & ZMQ_POLLIN) {
      cerr << "From clients\n";
      zmsg_t* msg = zmsg_recv(Clients);
      handleClientMessage(msg,Servers,Clients);
    }
  }

  zctx_destroy(&context);
	return 0;
}
//          PServer Pclient
// ./Broker 5555 4444
