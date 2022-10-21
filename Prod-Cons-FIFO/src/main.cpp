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
    consumidores = 6;


const int items = 172;

const int tamBuffer = 17;


// ---------------- VAR GLOBALES NO MODIFICABLES --------------------

int 
    testProduccion[items] = {0},
    testConsumicion[items] = {0};

//Reparto de trabjo en hebras
const int Cantidad_por_hebra_productores = items/productores;   
const int Cantidad_por_hebra_consumidores = items/consumidores; 
atomic<int> siguiente_dato;


class ProdCons : public HoareMonitor {
private:
    
    int buffer[tamBuffer];
    
    int elementos_en_buffer = 0;    //para controlar si está lleno o vacío
    int primera_libre = 0;
    int primera_ocupada = 0;
    
    CondVar puedeInsertar, puedeRetirar;
    
    
public:
    ProdCons(){
        puedeInsertar = newCondVar();
        puedeRetirar = newCondVar();
        
    }

    void escribirBufferFIFO(int dato,int num_hebra) {

        if (elementos_en_buffer == tamBuffer) 
            puedeInsertar.wait();
        
        buffer[primera_libre%tamBuffer] = dato;
        primera_libre++;
        
        elementos_en_buffer++;
        
        testProduccion[dato]++;
        
        puedeRetirar.signal();


    }  

    int retirarBufferFIFO(int num_hebra) {
       
        if (elementos_en_buffer == 0)    
            puedeRetirar.wait();
        
        cout << "hebra: " << num_hebra << " extraigo " << buffer[primera_ocupada % tamBuffer] << " en pos " << primera_ocupada % tamBuffer << endl;
        
        int dato = buffer[primera_ocupada%tamBuffer];
        primera_ocupada++;
        
        elementos_en_buffer--;
        
        testConsumicion[dato]++;


        puedeInsertar.signal();

        return dato;
    }
    
};

int producirDato(int num_hebra) {
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
    const unsigned dato_producido = siguiente_dato;
    siguiente_dato++;
    cout << "Hebra: " << num_hebra << " producido: " << dato_producido << endl;

    return dato_producido;
}

void consumirDato(int dato, int num_hebra) {
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));

    cout << "Hebra: " << num_hebra << " Consumido: " << dato << endl;
}


void HebraProductor(MRef<ProdCons> monitor, int num_hebra){
    int inicio = Cantidad_por_hebra_productores*num_hebra;
    int fin = Cantidad_por_hebra_productores + Cantidad_por_hebra_productores*num_hebra;
    
   

    if (num_hebra == productores - 1)
        fin = items;
    
    for(int i=inicio; i<fin; i++){
        int dato = producirDato(num_hebra);
        monitor->escribirBufferFIFO(dato,num_hebra);
    }
    

}

void HebraConsumidor( MRef<ProdCons> monitor, int num_hebra){
    int inicio = Cantidad_por_hebra_consumidores*num_hebra;     
    int fin = Cantidad_por_hebra_consumidores + Cantidad_por_hebra_consumidores*num_hebra;
    
    if (num_hebra == consumidores - 1)
        fin = items;
    
    for (unsigned i = inicio; i < fin; i++) {
        int dato = monitor->retirarBufferFIFO(num_hebra);
        consumirDato(dato,num_hebra);
    }
    
}

int main(int argc, char** argv) {

    MRef<ProdCons> monitor = Create<ProdCons>();


    thread hebrasproductoras[productores];
    thread hebrasconsumidoras[consumidores];

    for (int i = 0; i < productores; i++)
        hebrasproductoras[i] = thread(HebraProductor,monitor,i);

    for (int i = 0; i < consumidores; i++)
        hebrasconsumidoras[i] = thread(HebraConsumidor, monitor,i);

    for (int i = 0; i < productores; i++)
        hebrasproductoras[i].join();

    for (int i = 0; i < consumidores; i++)
        hebrasconsumidoras[i].join();

    
    for (int i = 0; i < items; i++)
        cout << "La producción " << i << " es: " << testProduccion[i]<< endl;
    
    for (int i=0; i < items; i++)
            cout << "La consumición " << i << " es :" <<testConsumicion[i] << endl;
    
    return 0;
    
   


}