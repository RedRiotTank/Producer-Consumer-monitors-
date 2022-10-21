/* 
 * File:   main.cpp
 * Author: albertoplaza
 *
 * Created on 5 de octubre de 2021, 10:00
 */

#include <iostream>
#include <cstdlib>
#include <chrono> 
#include "scd.h"
#include <mutex>
#include <thread>
#include<atomic>
using namespace std;
using namespace scd;

// ---------------- VAR GLOBALES MODIFICABLES --------------------

const int 
    productores = 3, 
    consumidores = 6,
    recicladores = 2;

const int items = 41;

const int 
    tamBuffer = 17,
    tamBufferRec = 4;

//Tiempos(ms)          
const int   
    minProdTime = 20,
    maxProdTime = 100,
    minConsTime = 20,
    maxConsTime = 100,
    minRecTime = 20,
    maxRecTime = 100;


// ---------------- VAR GLOBALES NO MODIFICABLES --------------------

int 
    testProduccion[items] = {0},
    testConsumicion[items] = {0},
    testProduccionBasura[items] = {0},
    testRecicladoBasura[items] = {0}; 

//Reparto de trabjo en hebras
const int Cantidad_por_hebra_productores = items/productores;   
const int Cantidad_por_hebra_consumidores = items/consumidores; 
const int cantidad_por_hebra_recicladores = items/recicladores;


atomic<int> siguiente_dato;


class ProdCons : public HoareMonitor {
private:
    
    int buffer[tamBuffer];
    

    int primera_libre = 0;
    
    CondVar puedeInsertar, puedeRetirar;
    
    
public:
    ProdCons(){
        puedeInsertar = newCondVar();
        puedeRetirar = newCondVar();
        
    }

    void escribirBufferLIFO(int dato,int num_hebra) {

        if (primera_libre == tamBuffer) 
            puedeInsertar.wait();
        
        buffer[primera_libre] = dato;
        primera_libre++;
        
        testProduccion[dato]++;
        
        puedeRetirar.signal();


    }  
    
   int retirarBufferLIFO(){
   
       if(primera_libre == 0)
           puedeRetirar.wait();
       
       
       primera_libre--;
       int dato = buffer[primera_libre];
       
       testConsumicion[dato]++;
       
       puedeInsertar.signal();
       
       return dato;
   }
};

class Recic : public HoareMonitor {

private:
    int bufferRec[tamBufferRec];
    
    int primera_libre_rec = 0;
    
    CondVar puedeInsertarRec, puedeRetirarRec;
    
public:
    Recic(){
        puedeInsertarRec = newCondVar();
        puedeRetirarRec = newCondVar();
    }
    
    void ProducirBasuraLIFO(int dato,int num_hebra){
       
       if (primera_libre_rec == tamBufferRec) 
            puedeInsertarRec.wait();
       
        cout << "introduzco " << dato << " en el buffer de reciclado en:"<< primera_libre_rec%tamBufferRec <<endl;
       
        bufferRec[primera_libre_rec] = dato;
        primera_libre_rec++;
      
        testProduccionBasura[dato]++;
   
        puedeRetirarRec.signal();
   
   }

    int retirarBasuraLIFO(int num_hebra) {

        if (primera_libre_rec == 0)
            puedeRetirarRec.wait();

        primera_libre_rec--;

        int dato = bufferRec[primera_libre_rec];

        
        testRecicladoBasura[dato]++;

        puedeInsertarRec.signal();

        return dato;

    }
};

int producirDato(int num_hebra) {
    this_thread::sleep_for(chrono::milliseconds(aleatorio<minProdTime, maxProdTime>()));
    
    const unsigned dato_producido = siguiente_dato;
    siguiente_dato++;
    
    cout << "Hebra: " << num_hebra << " producido: " << dato_producido << endl;

    return dato_producido;
}

void consumirDato(int dato, int num_hebra) {
    this_thread::sleep_for(chrono::milliseconds(aleatorio<minConsTime, maxConsTime>()));

    cout << "Hebra: " << num_hebra << " Consumido: " << dato << endl;
}

void reciclarDato(int dato, int num_hebra){
    this_thread::sleep_for(chrono::milliseconds(aleatorio<minRecTime, maxRecTime>()));
    cout << "Hebra: "<< num_hebra <<" Reciclado: " << dato << endl;
    
}


void HebraProductor(MRef<ProdCons> monitor, int num_hebra){
    int inicio = Cantidad_por_hebra_productores*num_hebra;
    int fin = Cantidad_por_hebra_productores + Cantidad_por_hebra_productores*num_hebra;
    
    if (num_hebra == productores - 1)
        fin = items;
    
    for(int i=inicio; i<fin; i++){
        int dato = producirDato(num_hebra);
        monitor->escribirBufferLIFO(dato,num_hebra);
    }   
}

void HebraConsumidor( MRef<ProdCons> monProdCons,MRef<Recic> monRecic,  int num_hebra){
    int inicio = Cantidad_por_hebra_consumidores*num_hebra;     
    int fin = Cantidad_por_hebra_consumidores + Cantidad_por_hebra_consumidores*num_hebra;
    
    if (num_hebra == consumidores - 1)
        fin = items;
    
    for (unsigned i = inicio; i < fin; i++) {
        int dato = monProdCons->retirarBufferLIFO();
        consumirDato(dato,num_hebra);
        monRecic->ProducirBasuraLIFO(dato,num_hebra);
    }
}

void HebraReciclador(MRef<Recic> monRecic,  int num_hebra){
    int inicio = Cantidad_por_hebra_consumidores*num_hebra;     
    int fin = Cantidad_por_hebra_consumidores + Cantidad_por_hebra_consumidores*num_hebra;
    
    if (num_hebra == recicladores - 1)
        fin = items;
    
    for (unsigned i = inicio; i < fin; i++) {
        int dato = monRecic->retirarBasuraLIFO(num_hebra);
        reciclarDato(dato,num_hebra);
    }
}

int main(int argc, char** argv) {

    MRef<ProdCons> monProdCons = Create<ProdCons>();
    MRef<Recic> monRecic = Create<Recic>();

    thread hebrasproductoras[productores];
    thread hebrasconsumidoras[consumidores];
    thread hebrasrecicladoras[recicladores];
    
    for (int i = 0; i < productores; i++)
        hebrasproductoras[i] = thread(HebraProductor,monProdCons,i);

    for (int i = 0; i < consumidores; i++)
        hebrasconsumidoras[i] = thread(HebraConsumidor, monProdCons,monRecic,i);

    for (int i = 0; i < recicladores; i++)
        hebrasrecicladoras[i] = thread(HebraReciclador, monRecic,i);
    
    for (int i = 0; i < productores; i++)
        hebrasproductoras[i].join();

    for (int i = 0; i < consumidores; i++)
        hebrasconsumidoras[i].join();
    
     for (int i = 0; i < recicladores; i++)
        hebrasrecicladoras[i].join();

    
    for (int i = 0; i < items; i++)
        cout << "La producción " << i << " es: " << testProduccion[i]<< endl;
    
    for (int i=0; i < items; i++)
            cout << "La consumición " << i << " es :" <<testConsumicion[i] << endl;
    
    for (int i=0; i < items; i++)
            cout << "introd recic " << i << " es :" <<testProduccionBasura[i] << endl;
    
    for (int i=0; i < items; i++)
            cout << "El reciclado " << i << " es :" <<testRecicladoBasura[i] << endl;
    
    return 0;
   
}