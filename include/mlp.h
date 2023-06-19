#ifndef MLP_H
#define MLP_H

#include <Eigen/Dense>
#include<vector>
#include<list>
#include<iostream>
#include"capa.h"
#include<cstdio>
using namespace std;
using namespace Eigen;

struct MLP {
    int n;
    int hidden_size;
    vector<Capa> capas;
    VectorXd salida;
    MatrixXd X, Y;

    VectorXd producto_recursivo(const int, const int, const int);
    void derivadas_oculta(const int, const int, const int, MatrixXd&, VectorXd&);
    void derivadas_salida(const int, MatrixXd&, VectorXd&);
    double coste(const VectorXd&, const VectorXd&);
    VectorXd vector_activacion(const int, const int=-1);
    const VectorXd& vector_derivada_activacion(const int);
    MatrixXd& matriz_pesos(const int);
    VectorXd& vector_sesgo(const int);

    MLP(MatrixXd, MatrixXd);
    ~MLP();

    void agregar_capa(Capa);
    double propagacion_adelante(const int);
    void propagacion_atras(const int, const double);

    void entrenar(const int, const double);
    MatrixXd evaluar();
};


#endif //MLP_MLP_H
