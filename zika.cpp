/*
    Modelo computacional para simular o fluxo sanguíneo em artérias específicas do corpo humano.
    Nesse modelo, cada partícula representa uma pequena porção do fluido sanguíneo.
    A função updatePosition é responsável por calcular como cada partícula se move ao longo do tempo devido a diferentes forças e influências.
*/

#include "modelagemZika.h" // funcoes utilizadas na modelagem
#include <iostream> // funcoes para saida
#include <fstream> // funcoes para manipulacao de arquivos

// #define DEBUG_MODE // comentar esta linha caso nao esteja em modo debug
#ifdef DEBUG_MODE
    #define RANDOM_SEED_CONSTANT 42
#endif

using namespace std;

/* Constantes utilizadas na simulacao */

const int NUM_ARTERIAS = 13;
const double comprimentoArteria[] = {1, 10, 3, 3, 3.5, 3.5, 16.75, 13.5, 39.75, 22, 22.25, 4, 19.25, 5.5, 10.5, 7.25, 3.5, 13.5, 39.75, 22.25, 22, 2, 2, 6.5, 5.75, 5.5, 5.25, 5, 1.5, 3, 1.5, 3, 12.5, 3.75, 8, 5.75, 14.5, 4.5, 11.25, 44.25};
const double resistenciaVascular[] = {1.502, 0.3, 1.42, 1.342, 0.7, 0.407, 0.4, 0.2, 0.25, 0.175, 0.175, 1.246, 0.4, 1.124, 0.924, 0.5, 0.407, 0.2, 0.25, 0.175, 0.175, 0.3, 0.25, 0.25, 0.15, 0.2, 0.838, 0.35, 0.814, 0.275, 0.792, 0.275, 0.627, 0.175, 0.55, 0.37, 0.314, 0.2, 0.2, 0.2};
const double totalD[] = {0, 1, 4, 7, 11, 16.5, 27, 32.25, 33.75, 35.25, 47.75, 55.75, 61.5};
const double elasticidadeParede[4][13] = { 
    {2.80255395, 2.70463342, 2.73086925, 2.70044097, 3.09967097, 3.09625142, 3.05128033, 2.37339617, 1.99734367, 2.3752644 , 2.68141248, 2.95806939, 4.58291478},
    {3.37870718, 3.32129288, 3.52361929, 3.61554463, 4.21037942, 4.50704167, 4.56984255, 3.67562146, 3.15379636, 3.82559375, 4.35170993, 4.92655864, 7.74529303},
    {3.69878565, 3.64307815, 3.8862281 , 4.00476166, 4.67167833, 5.04437305, 5.13396467, 4.14835062, 3.56917331, 4.34186109, 4.9445025 , 5.61936806, 8.85432724},
    {3.80334596, 3.74537948, 3.99331575, 4.11346024, 4.79770086, 5.17621006, 5.26625837, 4.25338507, 3.65858461, 4.44940732, 5.0664316 , 5.75579298, 9.06732555}
};
const int rota[] = {0, 2, 3, 11, 13, 14, 26, 28, 30, 32, 34, 35, 37};

const double INCREMENTO_TEMPO = 1e-1;
const double coeficienteDifusao = 1e-2;
const double pressaoInicial = 2 * resistenciaVascular[0];
const double pressaoVenosa = 1e-2;
const double resistenciaVenosa = 1.0;
const double fracaoAbsorvida = 0.02;
const double fracaoRefletida = 1 - fracaoAbsorvida;
const double limiteY = 1e-3;
const double passoY = 1e-3;

const vector<int> estagio1 = {6558050, 7906264, 8655256, 8899930};
const vector<int> estagio2 = {13201581, 36097470, 53530880, 43530297};
const vector<int> estagio3 = {2627406, 7205390, 10647480, 8617887};
const vector<vector<int>> ESTAGIOS = {estagio1, estagio2, estagio3};

/* Geradores de numeros aleatorios */
default_random_engine geradorDistNormal;
normal_distribution<double> distribuicaoNormal(0, sqrt(INCREMENTO_TEMPO));

default_random_engine geradorAbsorcaoReflexao;
discrete_distribution<int> distribuicaoAbsorcaoReflexao(fracaoAbsorvida, fracaoRefletida);

/* Variaveis utilizadas na simulacao */
int semana = 0;
int quantidadeParticulas = 1000000;
int particulaAtual = 0;
double pressaoHemodinamica = 0.0;
bool primeiraASerRecebida = false;
int arteriaAtual = 0;
vector<double> perfilVelocidade[NUM_ARTERIAS];



const string resultFile1 = "resultados/N0_S0_particleTime.csv";
const string resultFile2 = "resultados/N0_S0_first_delay.csv";    
const string resultFile3 = "resultados/N0_S0_received_particles.csv";

ofstream resultsFile1;
ofstream resultsFile2;  
ofstream resultsFile3;

int contadorAbsorvidas = 0;
int contadorTempoParticula = 0;
int contadorParticulaIrma = 0;


void updatePosition(double x, double y)
{
    int indiceY = (int) (y * 1000);
    
    double deltaX = 0; // deslocamento horizontal
    double deltaY = 0; // deslocamento vertical
    double incrementoWiener = distribuicaoNormal(geradorDistNormal);
    double* firstSpeedPointer = perfilVelocidade[arteriaAtual].data();   

    /* Calculando a posicao */
    
    deltaX = calcularDeslocamentoHorizontal(elasticidadeParede[semana][arteriaAtual], firstSpeedPointer[indiceY], INCREMENTO_TEMPO, coeficienteDifusao, incrementoWiener);
    int forca = 0;
    if (y > resistenciaVascular[rota[arteriaAtual]] or y < resistenciaVascular[rota[arteriaAtual]])
        forca = 1;
    else
    {
        srand(time(NULL));
        #ifdef DEBUG_MODE
            srand(RANDOM_SEED_CONSTANT);
        #endif

        if (rand() % 2 == 1) forca = -1;
        else forca = 1;
    }
    
    deltaY = calcularDeslocamentoVertical(forca, pressaoVenosa, INCREMENTO_TEMPO, coeficienteDifusao, incrementoWiener);

    x = x + deltaX;
    y = y + deltaY;
    
    /* Fim do calculo da posicao */

    contadorTempoParticula++;
    
    /* Inicio determinacao se Absorcao ou Reflexao */
    if (atendeCriterioAbsorcaoReflexao(resistenciaVascular, rota, arteriaAtual, y))
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
    if (atendeCriterioParadaX(totalD, comprimentoArteria, rota, arteriaAtual, x))
    {
        if (!primeiraASerRecebida)
        {
            resultsFile2 << rota[arteriaAtual] << "," << contadorTempoParticula * INCREMENTO_TEMPO << ",\n";
        }
        if (arteriaAtual == NUM_ARTERIAS - 1)
        {   
            resultsFile1 << contadorTempoParticula * INCREMENTO_TEMPO << ",\n";
            primeiraASerRecebida = true;

            return;
        }
        else
        {
            arteriaAtual++;

            /* Artéria irmã - Início */
            if (atendeCriterioParadaY(resistenciaVascular, rota, arteriaAtual, y))
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
    #ifdef DEBUG_MODE
        srand(RANDOM_SEED_CONSTANT);
    #endif


    float initialX = 0.0;
    float initialY = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (pressaoInicial - 2 * limiteY))) + limiteY;

    cout << "Número de artérias = " << NUM_ARTERIAS << '\n';

    for (int i = 0; i < NUM_ARTERIAS; i++)
    {
        cout << "Artéria " << i << '\n';
        pressaoHemodinamica = 2 * resistenciaVascular[rota[i]];

        for (int j = 0; j < static_cast<int>((pressaoHemodinamica + passoY) / passoY); j++)
        {
            double velocidadePontoAtual = calculaPerfilVelocidadePonto(resistenciaVenosa, passoY, pressaoHemodinamica, j);
            perfilVelocidade[i].push_back(velocidadePontoAtual);
        }
    }

    for (int i = 0; i < quantidadeParticulas; i++)
    {
        cout << "Partícula " << i << '/' << quantidadeParticulas <<  ' ' << (double) i / quantidadeParticulas * 100 << '%' << '\n';

        updatePosition(initialX, initialY);

        if (contadorAbsorvidas == 0 && contadorParticulaIrma == 0)
        {
            resultsFile3 << i << "," << rota[arteriaAtual] << "," << contadorTempoParticula * INCREMENTO_TEMPO << ",\n";
        }      

        particulaAtual++;
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

int main()
{
    createFolder("resultados");
    routine();
    return 0;
}
