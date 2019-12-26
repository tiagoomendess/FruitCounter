//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2016/2017
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//                       CÓDIGO AULAS
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define VC_DEBUG
#define MAX(a,b)(a>b?a:b)

//Estrutura para representar cor, assume por defeito 3 canais e 255 niveis
//Também possui um campo chamado gray que converte automaticamente de rgb para grayscale
typedef struct {
	unsigned char r, g, b;
	unsigned char gray;
}COLOR;

typedef enum
{
	UNDEFINED, SQUARE , CIRCLE 
} Shape;

typedef enum
{
	NADA, ARROWLEFT, ARROWRIGHT, ARROWUP, CAR, FORBIDDEN, HIGHWAY, STOP
} Signal;

typedef enum
{
	ERROR, SUCCESS, WARNING, INFO
} LogType;

typedef enum
{
	INDEFINIDO, LARANJA, MACA_VERMELHA, MACA_VERDE
} TipoFruta;

typedef struct {

	int id;
	TipoFruta tipo;
	int area;
	int sumArea;
	int sumPeri;
	int totAmostras;
	int label;
	int x;

} Fruta;

/* typedef enum
{
	false = 0, true = 1
} boolean;*/

typedef int boolean;
#define true 1
#define false 0


void MyLog(LogType tipo, char *mensagem);
int drawBoundingBox(IVC *imagem, OVC *blobs, int *nlabels, COLOR *color);
COLOR* newColor(int r, int g, int b);
int paintPixel(IVC *imagem, int position, COLOR *cor);
int meanBlur(IVC *src, IVC *dst, int kernel);
int drawGravityCentre(IVC *imagem, OVC *blobs, int *nlabels, COLOR *color);
IVC *vc_image_copy(IVC *src);
int blueSignalsToBinary(IVC *src, IVC *dst, IVC *hsvImage);
int redSignalsToBinary(IVC *src, IVC *dst, IVC *hsvImage);
int analisa(char *caminho);
int getSignals(IVC *src, IVC *dst, IVC *hsvImage);
IVC* sumBinaryImages(IVC *imagem1, IVC *imagem2);
Shape getBlobShape(OVC blob, IVC *image);
int analisaBlobs(OVC *blobs, int nlabels, IVC *labels, IVC *original, IVC *binaryImage, IVC* hsvImage);
Signal identifySignal(OVC blob, Shape shape, IVC *original, IVC *binaria, IVC *hsvImage);
boolean isBlue(int hue, int sat, int value);
boolean isRed(int hue, int sat, int value);
int hue360To255(int hue);
int hue255To360(int hue);
int value100To255(int value);
int value255To100(int value);

int vc_rgb_to_hsv(IVC *srcdst);
int value100To255(int value);
int value255To100(int value);

int vc_binary_dilate(IVC *src, IVC *dst, int size);
int vc_binary_erode(IVC *src, IVC *dst, int size);
int vc_binary_open(IVC *src, IVC *dst, int sizeErode, int sizeDilate);
int vc_binary_close(IVC *src, IVC *dst, int sizeErode, int sizeDilate);

int vc_gray_dilate(IVC *src, IVC *dst, int size);
int vc_gray_erode(IVC *src, IVC *dst, int size);
int vc_gray_open(IVC *src, IVC *dst, int sizeErode, int sizeDilate);
int vc_gray_close(IVC *src, IVC *dst, int sizeErode, int sizeDilate);

int vc_gray_to_binary_mean(IVC *srcdst);
int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel);
int vc_gray_to_binary(IVC *srcdst, int threshold);
int vc_gray_to_binary_bernsen(IVC *src, IVC *dst, int kernel);
int vc_gray_to_binary_adapt(IVC *src, IVC *dst, int kernel);

int vc_gray_negative(IVC *srcdst);
int vc_rgb_negative(IVC *srcdst);
int vc_red_filter(IVC *srcdst);
int vc_remove_red(IVC *srcdst);
int vc_rgb_to_gray(IVC *src, IVC *dst);

int vc_rgb_to_gray(IVC *src, IVC *dst);
int vc_rgb_to_gray_mean(IVC *src, IVC *dst);

//Funções FILTROS
int vc_gray_lowpass_mean_filter(IVC *src, IVC *dst, int kernel);
int vc_gray_lowpass_median_filter(IVC *src, IVC *dst, int kernel);

//Segundo trabalho
int frame_to_binary(IVC *ivcFrame, IVC *hsvImage, IVC *binaryImage);
int IplImage_to_IVC(IplImage *src, IVC *dst);
int IVC_to_IplImage(IVC *src, IplImage *dst);
int ordena_array_asc(int *array, int tamanho);
int get_median(int *array, int tamanho);
IplImage *new_ipl_image(int width, int height, int channels);
int copy_image_data(IplImage *src, IplImage *dst);
int vc_copy_image_data(IVC *src, IVC *dst);
OVC *vc_copy_blobs(OVC* src, int n);

int get_fruits(IVC *hsvFrame, IVC *dstBinary);

bool mass_centerA_in_boxB(OVC a, OVC b);
bool is_circle(IVC *image, OVC blob);
bool mass_centerA_in_boxB(OVC a, OVC b);

bool isOrange(int h, int s, int v);
bool isRedApple(int h, int s, int v);
bool isGreen(int h, int s, int v);
TipoFruta getFrutaType(IVC *hsvImage, IVC *binaryImage, OVC blob);
int insereFruta(IVC *hsvImage, IVC *binaryImage, OVC blob, Fruta *frutas, int *nfrutas);
int analisa_blobs(IVC* hsvFrame, IVC *binaryFrame, OVC *blobs, int nlabels, OVC *blobsOld, int nlabelsOld, Fruta *frutas, int *nFrutas);
bool check_blob(OVC blob, IVC *hsvFrame);
void imprime_relatorio(Fruta *frutas, int nFrutas);




