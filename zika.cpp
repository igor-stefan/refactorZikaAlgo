/*
    Modelo computacional para simular o fluxo sanguíneo em artérias específicas do corpo humano.
    Nesse modelo, cada partícula representa uma pequena porção do fluido sanguíneo.
    A função updatePosition é responsável por calcular como cada partícula se move ao longo do tempo devido a diferentes forças e influências.
*/

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <random>
#include <fstream>
#include <sys/stat.h>

using namespace std;

/* Parâmetros Iniciais - Início */

const int NUM_ARTERIAS = 13;
int arteriaAtual = 0;

const double comprimentoArteria[] = {1, 10, 3, 3, 3.5, 3.5, 16.75, 13.5, 39.75, 22, 22.25, 4, 19.25, 5.5, 10.5, 7.25, 3.5, 13.5, 39.75, 22.25, 22, 2, 2, 6.5, 5.75, 5.5, 5.25, 5, 1.5, 3, 1.5, 3, 12.5, 3.75, 8, 5.75, 14.5, 4.5, 11.25, 44.25};
const double resistenciaVascular[] = {1.502, 0.3, 1.42, 1.342, 0.7, 0.407, 0.4, 0.2, 0.25, 0.175, 0.175, 1.246, 0.4, 1.124, 0.924, 0.5, 0.407, 0.2, 0.25, 0.175, 0.175, 0.3, 0.25, 0.25, 0.15, 0.2, 0.838, 0.35, 0.814, 0.275, 0.792, 0.275, 0.627, 0.175, 0.55, 0.37, 0.314, 0.2, 0.2, 0.2};
const double totalD[] = {0, 1, 4, 7, 11, 16.5, 27, 32.25, 33.75, 35.25, 47.75, 55.75, 61.5};
const double elasticidadeParede[4][13] = { 
    {2.80255395, 2.70463342, 2.73086925, 2.70044097, 3.09967097, 3.09625142, 3.05128033, 2.37339617, 1.99734367, 2.3752644 , 2.68141248, 2.95806939, 4.58291478},
    {3.37870718, 3.32129288, 3.52361929, 3.61554463, 4.21037942, 4.50704167, 4.56984255, 3.67562146, 3.15379636, 3.82559375, 4.35170993, 4.92655864, 7.74529303},
    {3.69878565, 3.64307815, 3.8862281 , 4.00476166, 4.67167833, 5.04437305, 5.13396467, 4.14835062, 3.56917331, 4.34186109, 4.9445025 , 5.61936806, 8.85432724},
    {3.80334596, 3.74537948, 3.99331575, 4.11346024, 4.79770086, 5.17621006, 5.26625837, 4.25338507, 3.65858461, 4.44940732, 5.0664316 , 5.75579298, 9.06732555}
};

const int arteriaPai[] = {-1, 0, 0, 2, 2, 4, 4, 5, 5, 8, 8, 3, 3, 11, 13, 13, 11, 16, 16, 18, 18, 14, 21, 21, 22, 22, 14, 26, 26, 28, 28, 30, 30, 32, 32, 34, 35, 35, 36, 36};
const int arteriaIrmao[] = {-1, 2, 1, 4, 3, 6, 5, 8, 7, 10, 9, 12, 11, 16, 15, 14, 13, 18, 17, 20, 19, 26, 23, 22, 25, 24, 21, 28, 27, 30, 29, 32, 31, 34, 33, 35, 37, 36, 39, 38};
const int rota[] = {0, 2, 3, 11, 13, 14, 26, 28, 30, 32, 34, 35, 37};

const double INCREMENTO_TEMPO = 1e-1;
const double coeficienteDifusao = 1e-2;
double pressaoHemodinamica = 0.0;
const double pressaoInicial = 2 * resistenciaVascular[0];
const double pressaoVenosa = 1e-2;
const double resistenciaVenosa = 1.0;
const double fracaoAbsorvida = 0.02;
const double fracaoRefletida = 1 - fracaoAbsorvida;
const double limiteY = 1e-3;
const double passoY = 1e-3;

vector<double> perfilVelocidade[NUM_ARTERIAS];

default_random_engine geradorDistNormal;
normal_distribution<double> distribuicaoNormal(0, sqrt(INCREMENTO_TEMPO));

default_random_engine geradorAbsorcaoReflexao;
discrete_distribution<int> distribuicaoAbsorcaoReflexao(fracaoAbsorvida, fracaoRefletida);

int weeks = 0;
long int totalParticles = 1000000;
int currentParticle = 0;

bool isFirstReceived = false;

const vector<int> estagio1 = {6558050, 7906264, 8655256, 8899930};
const vector<int> estagio2 = {13201581, 36097470, 53530880, 43530297};
const vector<int> estagio3 = {2627406, 7205390, 10647480, 8617887};

const vector<vector<int>> ESTAGIOS = {estagio1, estagio2, estagio3};

const string folder1 = "particle_time";
const string folder2 = "first_delay";
const string folder3 = "received_particles";

const vector<string> folderNames = {folder1, folder2, folder3};

const string resultFile1 = "resultados/N0_S0_particleTime.csv";
const string resultFile2 = "resultados/N0_S0_first_delay.csv";    
const string resultFile3 = "resultados/N0_S0_received_particles.csv";

ofstream resultsFile1;
ofstream resultsFile2;  
ofstream resultsFile3;

vector<double> particleTimes(totalParticles, 0);

int contadorAbsorvidas = 0;
int contadorTempoParticula = 0;
int contadorParticulaIrma = 0;

/* Parâmetros Iniciais - Fim */


double calcularDeslocamentoHorizontal(double pressaoHemodinamica, double velocidadeInicial, double incrementoTempo, double coeficienteDifusao, double incrementoWiener){
    /*
        É influenciado pela pressão hemodinâmica,
        velocidade inicial -> (firstSpeedPointer[indiceY]),
        e um termo relacionado à difusão (sqrt(2 * coeficienteDifusao) * incrementoWiener).
        O movimento é proporcional à pressão hemodinâmica e à velocidade da partícula, e inclui uma componente aleatória modelada pela distribuição de Wiener.
    */

    return pressaoHemodinamica * velocidadeInicial * incrementoTempo + sqrt(2 * coeficienteDifusao) * incrementoWiener;
}

double calcularDeslocamentoVertical(int forca, double pressaoVenosa, double incrementoTempo, double coeficienteDifusao, double incrementoWiener){
    /*
        É influenciado pela pressão venosa, um termo relacionado à difusão -> (sqrt(2 * coeficienteDifusao) * incrementoWiener), e uma força determinada por condições na artéria.
        A força é definida com base na posição atual (y) em relação à resistência vascular da artéria (resistenciaVascular[rota[arteriaAtual]]).
        A direção da força é determinada aleatoriamente se a posicao da partícula for igual à resistência vascular.
    */

    return forca * pressaoVenosa * incrementoTempo + sqrt(2 * coeficienteDifusao) * incrementoWiener;
}

bool atendeCriterioParadaY(const double resistenciaVascular[], const int rota[], int arteriaAtual, double posYAtual){
    return posYAtual >= 2 * resistenciaVascular[rota[arteriaAtual]];
}

bool atendeCriterioParadaX(const double totalD[], const double comprimentoArteria[], const int rota[], int arteriaAtual, double posXAtual){
    return posXAtual >= totalD[arteriaAtual] + comprimentoArteria[rota[arteriaAtual]];
}

bool atendeCriterioAbsorcaoReflexao(const double resistenciaVascular[], const int rota[], const int arteriaAtual, double posAtualY){
    return posAtualY >= 2 * resistenciaVascular[rota[arteriaAtual]] or posAtualY <= 0;
}

void updatePosition(double x, double y)
{
    int indiceY = (int) (y * 1000);
    
    double deltaX = 0; // deslocamento horizontal
    double deltaY = 0; // deslocamento vertical
    double incrementoWiener = distribuicaoNormal(geradorDistNormal);
    double* firstSpeedPointer = perfilVelocidade[arteriaAtual].data();   

    /* Calculando a posicao */
    
    deltaX = pressaoHemodinamica * firstSpeedPointer[indiceY] * INCREMENTO_TEMPO + sqrt(2 * coeficienteDifusao) * incrementoWiener;
 
    int forca = 0;
    if (y > resistenciaVascular[rota[arteriaAtual]] or y < resistenciaVascular[rota[arteriaAtual]])
        forca = 1;
    else
    {
        srand(time(NULL)); 
        if (rand() % 2 == 1) forca = -1;
        else forca = 1;
    }

    deltaY = forca * pressaoVenosa * INCREMENTO_TEMPO + sqrt(2 * coeficienteDifusao) * incrementoWiener;

    x = x + deltaX;
    y = y + deltaY;
    
    /* Fim do calculo da posicao */

    contadorTempoParticula++;
    
    /* Inicio determinacao se Absorcao ou Reflexao */
    if (y >= 2 * resistenciaVascular[rota[arteriaAtual]] or y <= 0)
    {
        int ehReflexao = distribuicaoAbsorcaoReflexao(geradorAbsorcaoReflexao);

        if (ehReflexao) // Reflexao
        {
            y = y - 2 * deltaY; 
        }
        else // Absorcao
        {
            contadorAbsorvidas++;
            return;
        }
    }
    /* Fim determinacao se Absorcao ou Reflexao */

    /* Artéria e Critério de Parada */
    if (x >= totalD[arteriaAtual] + comprimentoArteria[rota[arteriaAtual]])
    {
        if (!isFirstReceived)
        {
            resultsFile2 << rota[arteriaAtual] << "," << contadorTempoParticula * INCREMENTO_TEMPO << ",\n";
        }
        if (arteriaAtual == NUM_ARTERIAS - 1)
        {   
            resultsFile1 << contadorTempoParticula * INCREMENTO_TEMPO << ",\n";
            isFirstReceived = true;

            return;
        }
        else
        {
            arteriaAtual++;

            /* Artéria irmã - Início */
            if (y >= 2 * resistenciaVascular[rota[arteriaAtual]])
            {
                contadorParticulaIrma++;

                return;
            }
            /* Artéria irmã - Fim */
        }
    }
    updatePosition(x, y);
    /* Fim da Artéria e Critério de Parada - Fim*/

    return;
}


int routine()
{
    cout << "ROTINA\n";
    resultsFile1.open(resultFile1);
    resultsFile1 << "Time (s),\n";

    resultsFile2.open(resultFile2);   
    resultsFile2 << "artery" << "," << "time" << ",\n";

    resultsFile3.open(resultFile3);
    resultsFile3 << "particle" << "," << "artery" << "," << "time" << ",\n";

    srand(time(NULL));

    float initialX = 0.0;
    float initialY = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (pressaoInicial - 2 * limiteY))) + limiteY;

    cout << "Número de artérias = " << NUM_ARTERIAS << '\n';

    for (int i = 0; i < NUM_ARTERIAS; i++)
    {
        cout << "Artéria " << i << '\n';
        pressaoHemodinamica = 2 * resistenciaVascular[rota[i]];

        for (int j = 0; j < static_cast<int>((pressaoHemodinamica + passoY) / passoY); j++)
        {
            perfilVelocidade[i].push_back((12 / (1.4 * resistenciaVenosa)) * (-(j * passoY / pressaoHemodinamica) + (1 - exp(-resistenciaVenosa * (j * passoY / pressaoHemodinamica))) / (1 - exp(-resistenciaVenosa))));
        }
    }

    for (int i = 0; i < totalParticles; i++)
    {
        cout << "Partícula " << i << '/' << totalParticles <<  ' ' << (double) i / totalParticles * 100 << '%' << '\n';

        updatePosition(initialX, initialY);

        if (contadorAbsorvidas == 0 && contadorParticulaIrma == 0)
        {
            resultsFile3 << i << "," << rota[arteriaAtual] << "," << contadorTempoParticula * INCREMENTO_TEMPO << ",\n";
        }      

        currentParticle++;
        initialX = 0.0;
        initialY = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (pressaoInicial - 2 * limiteY))) + limiteY;

        arteriaAtual = 0;
        contadorAbsorvidas = 0;
        contadorTempoParticula = 0;
        contadorParticulaIrma = 0;
    }

    cout << "Salvando resultados..." << endl;

    resultsFile1.close();
    resultsFile2.close();
    resultsFile3.close();

    return 0;
}

void createFolder(const char* folderName)
{
    struct stat st;
    if (stat(folderName, &st) == 0)
    {
        // Se o diretório já existe, remove-o
        std::string command = "rm -rf " + std::string(folderName);
        system(command.c_str());
    }
    int status = mkdir(folderName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == 0) 
        printf("%s folder created\n", folderName);
    else 
        printf("Error creating %s folder\n", folderName);
}

int main()
{
    createFolder("resultados");
    routine();
    return 0;
}
