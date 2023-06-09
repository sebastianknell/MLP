#include "mlp.h"

void MLP::asignar_datos_validacion(MatrixXd x_val, MatrixXd y_val){
    this->X_val = x_val;
    this->Y_val = y_val;
}
double MLP::perdida_validacion(){
    int filas = X_val.rows();
    VectorXd perdidas(filas);
    for(int f = 0; f<filas; f++){
        VectorXd vec_h = X_val.row(f).transpose();
        for(Capa c: this->capas){
            vec_h = c.evaluar(vec_h);
        }
        vec_h = this->softmax(vec_h);
        perdidas[f] = this->entropia(vec_h, Y_val.row(f));
    }
    return perdidas.mean();
}

VectorXd MLP::derivada_recursiva(const int capa_actual, const int capa_limite, const int neurona_destino){
    if(capa_actual == capa_limite){
        return vector_derivada_activacion(capa_limite)
            .cwiseProduct(matriz_pesos(capa_limite).transpose().col(neurona_destino));
    }
    return vector_derivada_activacion(capa_actual).cwiseProduct(matriz_pesos(capa_actual).transpose() 
        * this->derivada_recursiva(capa_actual-1, capa_limite, neurona_destino));
}

VectorXd MLP::softmax(const VectorXd& vec){
    VectorXd expo_vec = vec.array().exp();
    return expo_vec.array() / expo_vec.sum();
}

double MLP::propagacion_adelante(const int fila){
    VectorXd vec_h = X.row(fila).transpose();
    for(Capa &c: this->capas){
        vec_h = c.propagar(vec_h);
    }
    this->salida = this->softmax(vec_h);
    return this->entropia(salida, Y.row(fila));
}

void MLP::cargar(string ruta){
    int l;
    ifstream cantidad(ruta+"cantidad.txt");
    cantidad>>l;
    cantidad.close();

    int act_c;
    Activacion funcion;
    ifstream archivo_act(ruta+"activacion.csv");
    for(int i = 0; i<l; i++){
        archivo_act>>act_c;
        switch (act_c)
        {
        case 0:
            funcion = Activacion::sigmoidea;
            break;
        case 1:
            funcion = Activacion::tanh;
            break;
        case 2:
            funcion = Activacion::relu;
            break;
        }

        // Leer CSV
        MatrixXd pesos = cargar_csv(ruta + "pesos_capa_"+to_string(i)+".csv");

        Capa capa_i = Capa{(int) pesos.rows(),(int) pesos.cols(), funcion};
        capa_i.pesos = pesos;
        capa_i.sesgo = cargar_csv(ruta + "sesgo_capa_"+to_string(i)+".csv");

        this->agregar_capa(capa_i);

    }
    archivo_act.close();
}

void MLP::exportar(){
    int l = 0;

    ofstream archivo_act("../../data/activacion.csv", ios::out);
    for(const Capa& c: this->capas){

        switch(c.tipo){
            case Activacion::sigmoidea:
                archivo_act<<0<<'\n';
                break;
            case Activacion::tanh:
                archivo_act<<1<<'\n';
                break;
            case Activacion::relu:
                archivo_act<<2<<'\n';
                break;
        }

        ofstream archivo("../../data/pesos_capa_"+to_string(l)+".csv", ios::out);
        for(int fila = 0; fila<c.pesos.rows(); fila++){
            for(int columna = 0; columna<c.pesos.cols(); columna++){
                archivo<<c.pesos(fila, columna);
                if(columna != c.pesos.cols() -1)
                    archivo<<',';
            }
            archivo<<'\n';
        }
        archivo.close();
        archivo.open("../../data/sesgo_capa_"+to_string(l)+".csv", ios::out);
        for(int i = 0; i<c.sesgo.size(); i++){
            archivo<<c.sesgo[i]<<'\n';
        }
        archivo.close();
        l++;
    }
    archivo_act.close();

    ofstream cantidad("../../data/cantidad.txt");
    cantidad<<l;
    cantidad.close();
}

void MLP::derivadas_salida(const int fila, MatrixXd& derivada_pesos, VectorXd& derivada_sesgo){
    int indice_salida = this->capas.size()-1;
    VectorXd diferencia_y = this->vector_activacion(indice_salida) - this->Y.row(fila).transpose();
    VectorXd producto_elemento = diferencia_y.cwiseProduct(this->vector_derivada_activacion(indice_salida));
    
    derivada_sesgo = producto_elemento / this->Y.cols();
    derivada_pesos = vector_activacion(indice_salida-1) * derivada_sesgo.transpose();
}

void MLP::propagacion_atras(const int fila, const double ratio_aprendizaje) {
    // hidden output
    int n = this->capas.size();
    MatrixXd derivada_pesos;
    VectorXd derivada_sesgo;
    this->entropia_derivadas_salida(fila, derivada_pesos, derivada_sesgo);
    this->matriz_pesos(n-1) -= ratio_aprendizaje * derivada_pesos;
    this->vector_sesgo(n-1) -= ratio_aprendizaje * derivada_sesgo;
    // hidden hidden
    for (int l = n-2; l >= 0; l--) {
        // Luego se debe trasponer
        derivada_pesos = MatrixXd(this->matriz_pesos(l).rows(), this->matriz_pesos(l).cols());
        derivada_sesgo = VectorXd(this->vector_sesgo(l).size());
        // cout<<"NONONO"<<endl;
        for(int i = 0; i<derivada_pesos.cols(); i++){
            // cout<<l<<", "<<i<<endl;
            this->entropia_derivadas_oculta(l, i, fila, derivada_pesos, derivada_sesgo);
            // cout<<"Iteracion "<<derivada_pesos.cols()<<endl;
        }

        this->matriz_pesos(l) -= ratio_aprendizaje * derivada_pesos; 
        this->vector_sesgo(l) -= ratio_aprendizaje * derivada_sesgo;
    }
}

void MLP::derivadas_oculta(const int capa, const int neurona_destino, const int fila, MatrixXd& derivada_pesos, VectorXd& derivada_sesgo){
    int ultima_capa = this->capas.size()-1;
    VectorXd vector_y = this->Y.row(fila);
    VectorXd vector_h = this->vector_activacion(ultima_capa, fila);
    VectorXd diferencia_y = vector_h - vector_y;
    // cout<<capa<<endl;
    // cout<<"Recursivo Inicio"<<endl;
    VectorXd vector_recursion = derivada_recursiva(ultima_capa, capa, neurona_destino);
    // cout<<"Recursivo Fin"<<endl;
    
    // cout<<"Actualiza sesgo "<<capa<<" "<<neurona_destino<<endl;
    derivada_sesgo[neurona_destino] = vector_recursion.
        cwiseProduct(diferencia_y).mean() * vector_derivada_activacion(capa).coeff(neurona_destino);
    // cout<<"Actualiza pesos"<<endl;
    derivada_pesos.col(neurona_destino) = derivada_sesgo[neurona_destino] * vector_activacion(capa-1, fila);
}


void MLP::entrenar(const int epocas, const double ratio_aprendizaje) {
    int n = X.rows();
    ofstream archivo_p("../../data/perdidas.csv", ios::out);
    for (int epoca = 0; epoca < epocas; epoca++) {
        printf("Epoca N-%d -> ", epoca+1);
        VectorXd perdidas(n);
        double perdida_val = perdida_validacion();
        for (int i = 0; i < n; ++i) {
            double perdida = this->propagacion_adelante(i);
            perdidas[i] = perdida;
            this->propagacion_atras(i, ratio_aprendizaje);
        }
        printf("Perdida: %lf %lf\n", perdidas.mean(), perdida_val);
        archivo_p<<perdidas.mean()<<','<<perdida_val<<'\n';
    }
    archivo_p.close();
}

MatrixXd MLP::evaluar(){
    int filas = X.rows();
    MatrixXd resultado(this->Y.rows(), this->Y.cols());
    for(int i = 0; i<filas; i++){
        this->propagacion_adelante(i);
        resultado.row(i) = this->salida.transpose();
    }
    return resultado;
}

VectorXd MLP::vector_activacion(const int indice_capa, const int fila){
    if(indice_capa<0)
        return X.row(fila).transpose();
    return this->capas.at(indice_capa).activado;
}
const VectorXd& MLP::vector_derivada_activacion(const int indice_capa){
    return this->capas.at(indice_capa).derivada_activado;
}
MatrixXd& MLP::matriz_pesos(const int indice_capa){
    return this->capas.at(indice_capa).pesos;
}
VectorXd& MLP::vector_sesgo(const int indice_capa){
    return this->capas.at(indice_capa).sesgo;
}

double MLP::coste(const VectorXd& vec_h, const VectorXd& vec_y){
    return ((vec_h - vec_y).array().pow(2)/2).mean();
}

double MLP::entropia(const VectorXd& vec_h, const VectorXd& vec_y){
    VectorXd expo_h = vec_h.array().exp();
    VectorXd soft_h = expo_h / expo_h.sum();
    return -(vec_y.array() *soft_h.array().log()).sum();
}

void MLP::entropia_derivadas_salida(const int fila, MatrixXd& derivada_pesos, VectorXd& derivada_sesgo){
    int indice_salida = this->capas.size()-1;
    VectorXd diferencia_y = this->vector_activacion(indice_salida) - this->Y.row(fila).transpose();
    VectorXd producto_elemento = diferencia_y.cwiseProduct(this->vector_derivada_activacion(indice_salida));
    
    VectorXd vector_y = this->Y.row(fila);
    VectorXd vector_h = this->vector_activacion(indice_salida, fila);

    VectorXd soft_h = this->softmax(vector_h);

    VectorXd derivada_h_b = this->vector_derivada_activacion(indice_salida);
    derivada_sesgo = (soft_h - vector_y).cwiseProduct(derivada_h_b);    
    derivada_pesos = vector_activacion(indice_salida-1) * derivada_sesgo.transpose();
}

void MLP::entropia_derivadas_oculta(const int indice_capa, const int neurona_destino, const int indice_dato, MatrixXd& derivada_pesos, VectorXd& derivada_sesgo){
    int indice_ultima_capa = this->capas.size()-1;
    VectorXd vector_y = this->Y.row(indice_dato);
    VectorXd soft_h = this->softmax(this->vector_activacion(indice_ultima_capa));

    VectorXd vector_R = derivada_recursiva(indice_ultima_capa, indice_capa+1, neurona_destino);

    derivada_sesgo[neurona_destino] = (soft_h - vector_y).dot(vector_R * vector_derivada_activacion(indice_capa)[neurona_destino]);
    derivada_pesos.col(neurona_destino) = derivada_sesgo[neurona_destino] * vector_activacion(indice_capa-1, indice_dato);
}

MLP::MLP(MatrixXd X, MatrixXd Y): X(X), Y(Y) {};

void MLP::agregar_capa(Capa capa)
{
    this->capas.push_back(capa);
}

MLP::~MLP(){}