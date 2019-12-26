#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <string.h>

#include <opencv\cv.h>
#include <opencv\cxcore.h>
#include <opencv\highgui.h>

#include "vc.h"
#include "labelling.h"
#include "ca.h"

//FUNÇÔES FEITAS NA AULA ==============================================
#pragma region TRABALHO 2

void imprime_relatorio(Fruta *frutas, int nFrutas) {

	int aux;

	system("cls");

	printf("----- [ Laranjas ]----------------\n\n");
	aux = 0;
	for (int i = 0; i < nFrutas; i++)
	{
		if (frutas[i].tipo == LARANJA) {
			printf("[%d]: Area: %d Perimetro: %d. \n", frutas[i].id + 1, frutas[i].sumArea / frutas[i].totAmostras, frutas[i].sumPeri / frutas[i].totAmostras);
			aux++;
		}

	}
	printf("\n-> Sub-Total de Laranjas: %d\n\n", aux);

	printf("----- [ Macas Verdes ]------------\n\n");
	aux = 0;
	for (int i = 0; i < nFrutas; i++)
	{
		if (frutas[i].tipo == MACA_VERDE) {
			printf("[%d]: Area: %d Perimetro: %d. \n", frutas[i].id + 1, frutas[i].sumArea / frutas[i].totAmostras, frutas[i].sumPeri / frutas[i].totAmostras);
			aux++;
		}

	}
	printf("\n-> Sub-Total de Macas Verdes: %d\n\n", aux);

	printf("----- [ Macas Vermelhas ]---------\n\n");
	aux = 0;
	for (int i = 0; i < nFrutas; i++)
	{
		if (frutas[i].tipo == MACA_VERMELHA) {
			printf("[%d]: Area: %d Perimetro: %d. \n", frutas[i].id + 1, frutas[i].sumArea / frutas[i].totAmostras, frutas[i].sumPeri / frutas[i].totAmostras);
			aux++;
		}

	}
	printf("\n-> Sub-Total de Maca Vermelha: %d\n\n", aux);

	printf("-> Total: %d\n\n", nFrutas);

	return;

}

//passa o frame para binário
int frame_to_binary(IVC *ivcFrame, IVC *hsvImage, IVC *binaryImage) {

	IVC *erodeB = vc_image_new(ivcFrame->width, ivcFrame->height, 1, 255);

	int errorControl = 0;

	if (errorControl = 0)
	{
		MyLog(ERROR, "Erro ao passar o frame de RGB para HSV");
		return 0;
	}

	get_fruits(hsvImage, binaryImage);
	vc_binary_open(binaryImage, erodeB, binaryImage->width * 0.01f, binaryImage->width * 0.01f);
	
	vc_copy_image_data(erodeB, binaryImage);

	vc_image_free(erodeB);
	return 1;
}

//Basicamente faz o reconhecimento dos blobs
int analisa_blobs(IVC* hsvFrame, IVC *binaryFrame, OVC *blobs, int nlabels, OVC *blobsOld, int nlabelsOld, Fruta *frutas, int *nFrutas) {

	bool updated = false;

	for (int i = 0; i < nlabels; i++) //Percorrer novos blobs
	{

		if (!check_blob(blobs[i], hsvFrame)) //Se o blob nao passar nos checks, passamos ao próximo
			continue;

		for (int j = 0; j < nlabelsOld; j++) //Percorrer blobs do frame anterior
		{
			if (!check_blob(blobs[i], hsvFrame)) //Se o blob nao passar nos checks, passamos ao próximo
				continue;

			if (mass_centerA_in_boxB(blobsOld[j], blobs[i]) == true) //Verificar se o centro de massa do anterior esta dentro da caixa do atual
			{
				for (int k = 0; k < *nFrutas; k++)
				{

					if (frutas[k].area == blobsOld[j].area)
					{

						frutas[k].area = blobs[i].area;
						frutas[k].sumArea += blobs[i].area;
						frutas[k].sumPeri += blobs[i].perimeter;
						frutas[k].totAmostras++;
						frutas[k].x = blobs[i].x;
						updated = true;
						break;
					}
				}
			}

		}

		//Existe uma nova peça de fruta no frame
		if(!updated)
			insereFruta(hsvFrame, binaryFrame, blobs[i], frutas, nFrutas);
	}

	return 1;
}

//Verifica se um blob é uma possível fruta
bool check_blob(OVC blob, IVC *hsvFrame) {

	//Se o blob estiver muito colado acima ou abaixo do frame, ignorar
	if (blob.y < 2 || blob.y + blob.height > hsvFrame->height - 2)
		return false;

	//Se o blob for demasiado pequeno ou demasiado grande, descartar
	if (blob.width < hsvFrame->width * 0.10f || blob.width > hsvFrame->width * 0.80f)
		return false;

	//Se o blob for muito rectangular, descartar
	if (abs(blob.width - blob.height) > blob.width * 0.25f)
		return false;

	return true;
}

//Insere a peça de fruta no array de frutas
int insereFruta(IVC *hsvImage, IVC *binaryImage, OVC blob, Fruta *frutas, int *nfrutas) {

	TipoFruta tipo;

	//Se nao for um circulo é porque nao e uma das frutas
	if (!is_circle(binaryImage, blob)) return 0;

	//Descobrir qual a fruta
	tipo = getFrutaType(hsvImage, binaryImage, blob);

	if (tipo != INDEFINIDO)
	{
		frutas[*nfrutas].area = blob.area;
		frutas[*nfrutas].id = *nfrutas;
		frutas[*nfrutas].label = blob.label;
		frutas[*nfrutas].sumArea = blob.area;
		frutas[*nfrutas].sumPeri = blob.perimeter;
		frutas[*nfrutas].tipo = tipo;
		frutas[*nfrutas].x = blob.x;
		frutas[*nfrutas].totAmostras = 1;
		*nfrutas = *nfrutas + 1;
	}
	else
	{
		return 0;
	}

	return 1;
}

//Identifica o tipo de fruta
TipoFruta getFrutaType(IVC *hsvImage, IVC *binaryImage, OVC blob) {

	TipoFruta tipo = INDEFINIDO;

	int laranjas = 0;
	int vermelhos = 0;
	int verdes = 0;
	int x, posH, ymid, offset, h, s, v;
	ymid = blob.y + (blob.height / 2);
	offset = blob.width * 0.10f;

	//Vazer linha na horizontal a meio do blob
	for (x = blob.x + offset; x < blob.x + blob.width - offset; x++)
	{
		posH = x * hsvImage->channels + ymid * hsvImage->bytesperline;

		h = hue255To360(hsvImage->data[posH]);
		s = value255To100(hsvImage->data[posH + 1]);
		v = value255To100(hsvImage->data[posH + 2]);

		if (isOrange(h, s, v))
			laranjas++;

		if (isRedApple(h, s, v))
			vermelhos++;


		if (isGreen(h, s, v))
			verdes++;
	}

	if (vermelhos > laranjas && vermelhos > verdes)
		tipo = MACA_VERMELHA;
	else if (laranjas > vermelhos && laranjas > verdes)
		tipo = LARANJA;
	else
		tipo = MACA_VERDE;

	return tipo;


}

//Verifica se o blob passado e um circulo
bool is_circle(IVC *image, OVC blob) {

	int pos, xoff, yoff;

	xoff = blob.width * 0.10f;
	yoff = blob.height * 0.10f;

	//Verificar canto superior esquerdo
	pos = (blob.x + xoff) * image->channels + (blob.y + yoff) * image->bytesperline;
	if ((int)image->data[pos] == 255)
		return false;

	//Verificar canto inferior direito
	pos = (blob.x + blob.width - xoff) * image->channels + (blob.y + blob.height - yoff) * image->bytesperline;
	if ((int)image->data[pos] == 255)
		return false;

	return true;

}

//Verifica se o centro de massa do blob A está dentro da boundingbox do blob b
bool mass_centerA_in_boxB(OVC a, OVC b) {

	if (a.xc > b.x && a.xc < b.x + b.width) {
		if (a.yc > b.y && a.yc < b.y + b.height)
			return true;
		else
			return false;
	}

	return false;

}

//Faz uma cópia de um array de blobs
OVC *vc_copy_blobs(OVC* src, int n) {

	OVC *newArray = (OVC *)calloc(n, sizeof(OVC));

	memcpy(newArray, src, sizeof(OVC) * n);

	return newArray;
}

//Dada uma imagem hsv, retorna uma binaria so com as frutas
int get_fruits(IVC *hsvFrame, IVC *dstBinary) {

	//verificações
	if (hsvFrame->width != dstBinary->width || hsvFrame->height != dstBinary->height)
		return 0;

	if (dstBinary->channels > 1)
		return 0;

	int x, y, posH, posB;

	for (y = 0; y < hsvFrame->height; y++)
	{
		for (x = 0; x < hsvFrame->width; x++)
		{
			posH = x * hsvFrame->channels + y * hsvFrame->bytesperline;
			posB = x * dstBinary->channels + y * dstBinary->bytesperline;

			if (hsvFrame->data[posH + 1] < value100To255(26) && hsvFrame->data[posH + 2] > value100To255(32))
				dstBinary->data[posB] = (unsigned char) 0;
			else
				dstBinary->data[posB] = (unsigned char) 255;
			
		}
	}

	return 1;
}

int vc_copy_image_data(IVC *src, IVC *dst) {

	if (src->width != dst->width || src->height != dst->height)
		return 0;

	if (src->channels != dst->channels || src->levels != dst->levels)
		return 0;

	memcpy((void*)dst->data, (void*)src->data, src->width * src->height * src->channels);

	return 1;
}

//Verifica se a cor passada por parametro está dentro do range das laranjas
bool isOrange(int h, int s, int v) {

	if (h > 17 && h < 33 && s > 76 && s < 96 && v > 35 && v < 75)
		return true;
	else
		return false;
}

//Verifica se a cor passada por parametro está dentro do range das maçãs vermelhas
bool isRedApple(int h, int s, int v) {

	if (h > 0 && h < 18 && s > 64 && s < 90 && v > 35 && v < 55)
		return true;
	else
		return false;
}

//Verifica se a cor passada por parametro está dentro do range das maças verdes
bool isGreen(int h, int s, int v) {

	if (h > 48 && h < 68 && s > 36 && s < 78 && v > 1 && v < 80)
		return true;
	else
		return false;
}

//Comia o imageData de uma IplImage para outra
int copy_image_data(IplImage *src, IplImage *dst) {

	if (src == NULL)
		return 0;

	if (dst == NULL)
		return 0;

	memcpy(dst->imageData, src->imageData, src->nChannels * src->width * src->height);

	return 1;
}

//Copia o image data de uma IplImage para uma IVC
int IplImage_to_IVC(IplImage *src, IVC *dst) {

	if (dst == NULL || src == NULL)
		return 0;

	if (src->width != dst->width || src->height != dst->height)
		return 0;

	int x, y, pos;

	for (y = 0; y < src->height; y++)
	{
		for  (x = 0; x < src->width; x++)
		{
			pos = x * src->nChannels + y * (src->width * src->nChannels);

			dst->data[pos] = (unsigned char) src->imageData[pos + 2];
			dst->data[pos + 1] = (unsigned char) src->imageData[pos + 1];
			dst->data[pos + 2] = (unsigned char) src->imageData[pos];

		}
	}

	//memcpy((void*) dst->data, (void*) src->imageData, src->width * src->height * src->nChannels);

	return 1;
}

//Copia o image data de um IVC para um IplImage
int IVC_to_IplImage(IVC *src, IplImage *dst) {

	if (dst == NULL || src == NULL)
		return 0;

	if (src->width != dst->width || src->height != dst->height)
		return 0;

	int x, y, pos;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = x * src->channels + y * (src->width * src->channels);

			dst->imageData[pos] = (char) src->data[pos + 2];
			dst->imageData[pos + 1] = (char) src->data[pos + 1];
			dst->imageData[pos + 2] = (char) src->data[pos];

		}
	}

	//memcpy(dst->imageData, src->data, src->height * src->width * src->channels);

	return 1;
}

IplImage *new_ipl_image(int width, int height, int channels) {

	CvSize cvsize;
	cvsize.width = width;
	cvsize.height = height;
	return cvCreateImage(cvsize, CV_8U, channels);

}

#pragma endregion


#pragma region TRABALHO 1


int analisa(char *caminho) {

	MyLog(INFO, "A analizar imagem...");
	
	IVC *aAnalizar = vc_read_image(caminho);
	if (aAnalizar == NULL) {
		MyLog(ERROR, "A imagem nao foi encontrada.");
		return 0;
	}

	IVC *hsvImage;
	IVC *blured = vc_image_new(aAnalizar->width, aAnalizar->height, aAnalizar->channels, aAnalizar->levels);
	IVC *binary = vc_image_new(aAnalizar->width, aAnalizar->height, 1, 255);
	IVC *blobLabels = vc_image_new(aAnalizar->width, aAnalizar->height, 1, 255);
	Shape shape;
	int errorControl;
	int nlabels;
	OVC *blobs;
	int kernelSize = aAnalizar->width * 0.01f; //kernel de 2\%

	//Certificar que é um numero impar
	if ((kernelSize % 2) == 0)
		kernelSize++;

	printf("[INFO]: Kernel de %d \n", kernelSize);

	MyLog(INFO, "A aplicar filtros de melhoramento de imagem.");
	errorControl = meanBlur(aAnalizar, blured, kernelSize);

	hsvImage = vc_image_copy(blured);
	vc_rgb_to_hsv(hsvImage);

	vc_write_image("Resultados/BluredImage.ppm", blured);

	if (errorControl == 0)
	{
		MyLog(ERROR, "Erro ao aplicar filtro blur.");
		return 0;
	}

	MyLog(INFO, "A extrair sinais de transito.");
	errorControl = getSignals(blured, binary, hsvImage);

	if (errorControl == 0)
	{
		MyLog(ERROR, "Erro ao indentificar sinais. Metodo getSignals.");
		return 0;
	}

	MyLog(INFO, "A fazer etiquetagem.");
	blobs = vc_binary_blob_labelling(binary, blobLabels, &nlabels);

	vc_write_image("Resultados/labels.pgm", blobLabels);

	printf("[INFO]: Foram encontrados %d blobs. \n", nlabels);

	MyLog(INFO, "A recolher informacoes sobre os blobs.");
	errorControl = vc_binary_blob_info(blobLabels, blobs, nlabels);

	vc_write_image("Resultados/binary.pgm", binary);

	MyLog(INFO, "A analizar blobs.");
	analisaBlobs(blobs, nlabels, blobLabels, aAnalizar, binary, hsvImage);

	MyLog(INFO, "A Libertar memoria...");
	//Libertar memória
	vc_image_free(aAnalizar);
	vc_image_free(blured);
	vc_image_free(binary);
	vc_image_free(blobLabels);
	free(blobs);

	return 1;
}

//Vai analizar individualmente cada blob encontrado
int analisaBlobs(OVC *blobs, int nlabels, IVC *labels, IVC *original, IVC *binaryImage, IVC *hsvImage) {

	IVC *tmp;
	int x, y, i, reds, blues;
	Shape forma;
	Signal sinal;

	drawBoundingBox(original, blobs, &nlabels, newColor(255, 0, 0));
	vc_write_image("Resultados/boxes.ppm", original);
	//Percorrer todos os blobs encontrados
	for (i = 0; i < nlabels; i++)
	{
		

		//Só interessam blobs maiores que 10% da imagem e blobs menores que 90%
		if (blobs[i].width < (original->width * 0.10f) || blobs[i].width > (original->width * 0.95f))
			continue;

		//Só interessam blobs quadrados. 15 pixeis é a margem de erro
		if ((blobs[i].width - blobs[i].height) <= -15 || (blobs[i].width - blobs[i].height) >= 15)
			continue;

		MyLog(INFO, "Potencial sinal encontrado.");

		forma = getBlobShape(blobs[i], binaryImage);

		sinal = identifySignal(blobs[i], forma, original, binaryImage, hsvImage);

		switch (sinal)
		{
		case FORBIDDEN:
			MyLog(SUCCESS, "Sinal de proibido encontrado!");
			break;

		case STOP:
			MyLog(SUCCESS, "Sinal de STOP encontrado!");
			break;

		case ARROWLEFT:
			MyLog(SUCCESS, "Sinal de seta para a esquerda encontrado!");
			break;

		case ARROWRIGHT:
			MyLog(SUCCESS, "Sinal de seta para a direita encontrado!");
			break;

		case ARROWUP:
			MyLog(SUCCESS, "Sinal de seta para cima encontrado!");
			break;

		case CAR:
			MyLog(SUCCESS, "Sinal de carro encontrado!");
			break;

		case HIGHWAY:
			MyLog(SUCCESS, "Sinal de auto-estrada encontrado!");
			break;

		default:
			MyLog(WARNING, "Um blob que potencialmente era um sinal nao foi identificado.");
			break;
		}

	}

	//vc_image_free(tmp);
	return 1;
}

Signal identifySignal(OVC blob, Shape shape, IVC *original, IVC *binaria, IVC *hsvImage) {

	int x, y, i, posOriginal, posBinaria;
	int azuis = 0;
	int vermelhos = 0;
	int trocasCor = 0;
	float offset = 0.05f;
	int blobxMax = (int) (blob.x + blob.width) - (blob.width * offset);
	int blobxMin = (int) blob.x + (blob.width * offset);
	int yMid = blob.y + (blob.height / 2); //Meio da imagem horizontal
	int xMid = blob.x + (blob.width / 2);
	int lastColor = 255;
	int h, s, v;
	int deslocamentoX = xMid - blob.xc;
	int deslocamentoY = yMid - blob.yc;
	Signal sinal = NADA;

	printf("yx: %d - xc: %d\n", blob.yc, blob.xc);

	//Percorrer horizontal a meio do blob cortando 5% às lateriais
	for (x = blobxMin; x < blobxMax; x++)
	{
		posOriginal = yMid * original->bytesperline + x * original->channels;
		posBinaria = yMid * binaria->bytesperline + x * binaria->channels;

		h = hue255To360(hsvImage->data[posOriginal]);
		s = value255To100(hsvImage->data[posOriginal + 1]);
		v = value255To100(hsvImage->data[posOriginal + 2]);
		
		if (isBlue(h, s, v) == true)
			azuis++;

		if (isRed(h, s, v) == true)
			vermelhos++;

		if ((int)binaria->data[posBinaria] != lastColor) {
			trocasCor++;
			lastColor = (int)binaria->data[posBinaria];
		}

	}


	//Verificar Sinais vermelhos
	if (vermelhos > azuis)
	{

		if (shape == CIRCLE) //Sinais circulares
		{
			if (trocasCor < 3) // Sinal de proibido
				sinal = FORBIDDEN;

			if (trocasCor > 4) // Sinal de STOP
				sinal = STOP;
		}
	}
	else //Sinais azuis
	{
		if (shape == SQUARE) //Sinais quadrados
		{
			if (trocasCor < 5) //Auto-estrada
				sinal = HIGHWAY;

			if (trocasCor >= 5) //Carro
				sinal = CAR;
		}

		if (shape == CIRCLE) //Sinais circulares
		{
			if (abs(deslocamentoX) > abs(deslocamentoY)) //Significa que é uma seta para a direita ou para a esquerda
			{
				if (deslocamentoX < 0) //Seta para a esquerda
					sinal = ARROWLEFT;
				else //Seta para a direita
					sinal = ARROWRIGHT;
			}
			else //Significa que e uma seta para cima ou para baixo
			{
				if (deslocamentoY < 0) //Seta para cima
					sinal = ARROWUP;
				else
					sinal = NADA; //Não existe seta para baixo

			}
		}
	}

	//Verificar se é Auto- Estrada
	if (azuis > vermelhos && trocasCor < 5 && shape == SQUARE)
		sinal = HIGHWAY;

	//Verificar se é Carro
	if (azuis > vermelhos && trocasCor >= 5 && shape == SQUARE)
		sinal = CAR;

	//Setas
	if (azuis > vermelhos && shape == SQUARE)
	{

	}

	return sinal;
}

//Extrai apenas os sinais azuis para binario
int blueSignalsToBinary(IVC *src, IVC *dst, IVC *hsvImage) {

	int x, y, i, pos, posBinary;

	int hue, sat, value;

	if (dst->channels != 1)
		return 0;

	for (y = 0; y < hsvImage->height; y++)
	{
		for (x = 0; x < hsvImage->width; x++)
		{

			posBinary = y * dst->bytesperline + x * dst->channels;
			pos = y * hsvImage->bytesperline + x * hsvImage->channels;

			hue = hsvImage->data[pos];
			sat = hsvImage->data[pos + 1];
			value = hsvImage->data[pos + 2];

			if ((hue >= hue360To255(214) && hue <= hue360To255(269)) && (value >= value100To255(15) && value <= value100To255(60)) && (sat >= value100To255(15) && sat <= value100To255(100)))
				dst->data[posBinary] = 255;
			else
				dst->data[posBinary] = 0;

		}
	}

	return 1;
}

//Extrai apenas os sinais vermelhos para binario
int redSignalsToBinary(IVC *src, IVC *dst, IVC *hsvImage) {

	int x, y, i, pos, posBinary;
	int hue, sat, value;

	if (dst->channels != 1)
		return 0;

	for (y = 0; y < hsvImage->height; y++)
	{
		for (x = 0; x < hsvImage->width; x++)
		{

			posBinary = y * dst->bytesperline + x * dst->channels;
			pos = y * hsvImage->bytesperline + x * hsvImage->channels;

			hue = hsvImage->data[pos];
			sat = hsvImage->data[pos + 1];
			value = hsvImage->data[pos + 2];

			//Vermelho vai de 350 a 7º
			if ((hue >= hue360To255(350) && hue <= hue360To255(360)) || (hue >= hue360To255(0) && hue <= hue360To255(8))) {

				if ((sat >= value100To255(54) && sat <= value100To255(100)) && (value >= value100To255(31) && value <= value100To255(93)))
					dst->data[posBinary] = 255;
				else
					dst->data[posBinary] = 0;
			}
			else {
				dst->data[posBinary] = 0;
			}

		}
	}

	return 1;
}

//Verifica se é azul. Usar valores do HSV e não de 0-255
boolean isBlue(int hue, int sat, int value) {
	
	if ((hue >= 213 && hue <= 269) && (sat >= 15 && sat <= 100) && (value >= 15 && value <= 60))
		return true;
	else
		return false;

}

//Verifica se é vermelho. Usar valores do HSV e não de 0-255
boolean isRed(int hue, int sat, int value) {

	//Vermelho vai de 350 a 7º
	if ((hue >= 350 && hue <= 360) || (hue >= 0 && hue <= 8)) {

		if ((sat >= 54 && sat <= 100) && (value >= 31 && value <= 93))
			return true;
		else
			return false;
	}
	else {
		return false;
	}

}

//Diz que formato tem o blob
Shape getBlobShape(OVC blob, IVC *image) {

	int boxPerimeter = blob.width + blob.height + blob.width + blob.height; //Perimetro da caixa delimitadora do blob 
	int blobPerimeter = blob.perimeter;
	int bytesperline = image->bytesperline;
	int channels = image->channels;
	int difxy = (blob.width - blob.height);
	int tl, tr, bl, br; //top left, top right...
	float offset = 0.10f; //Distancia dentro da boundingbox que vai viajar para dentro
	Shape shape = UNDEFINED;

	tl = ((blob.y + (int) (blob.height * offset)) * bytesperline + ((blob.x + (int) (blob.width * offset)) * channels));


	if (difxy >= -10 && difxy <= 10)
	{
		//Se Encontrar branco é porque é um quadrado e se for preto e porque é um circulo
		if (image->data[tl] == 0)//Circulo
			shape = CIRCLE;
		else //Quadrado
			shape = SQUARE;
	}
	
	return shape;
}

//Retorna uma imagem binaria com os sinais a branco
int getSignals(IVC *src, IVC *dst, IVC *hsvImage) {

	IVC *blueSignals = vc_image_new(src->width, src->height, 1, 255);
	IVC *redSignals = vc_image_new(src->width, src->height, 1, 255);
	IVC *blueClosed = vc_image_copy(blueSignals);
	IVC *redClosed = vc_image_copy(redSignals);
	IVC *blueOpened = vc_image_copy(blueSignals);
	IVC *redOpened = vc_image_copy(redSignals);
	int kernelSize = blueSignals->width * 0.02f; //kernel de 2\%

	//Certificar que é um numero impar
	if ((kernelSize % 2) == 0)
		kernelSize++;

	//Converter sinais azuis para binario e verificar se nao houve problemas
	if (blueSignalsToBinary(src, blueSignals, hsvImage) == 0)
		return 0;

	//Fechar a imagem com o kernel definido
	if (vc_binary_close(blueSignals, blueClosed, kernelSize, kernelSize) == 0)
		return 0;

	vc_binary_open(blueSignals, blueOpened, kernelSize, kernelSize);

	//Converter vermelhos para binario e verificar se nao houve problemas
	if (redSignalsToBinary(src, redSignals, hsvImage) == 0)
		return 0;

	vc_write_image("Resultados/redSignals.ppm", redSignals);
	vc_write_image("Resultados/blueSignals.ppm", blueSignals);

	//Fechar a imagem com kernel definido
	if (vc_binary_close(redSignals, redClosed, kernelSize, kernelSize) == 0)
		return 0;

	vc_binary_open(redSignals, redOpened, kernelSize, kernelSize);

	vc_write_image("Resultados/redClosed.ppm", redClosed);
	vc_write_image("Resultados/redOpened.ppm", redOpened);
	vc_write_image("Resultados/blueOpened.ppm", blueOpened);
	vc_write_image("Resultados/blueClosed.ppm", blueClosed);

	//Somar as duas imagens binarias, para permitir reconhecer mais do que 1 sinal de cada imagem
	*dst = *sumBinaryImages(blueClosed, redClosed);

	vc_write_image("Resultados/dstEMGetSignals.pgm", dst);

	//verificar se nao houve problemas na soma
	if (dst == NULL)
		return 0;

	vc_image_free(blueSignals);
	vc_image_free(redSignals);
	vc_image_free(blueOpened);
	vc_image_free(redOpened);
	vc_image_free(blueClosed);
	vc_image_free(redClosed);

	return 1;
}

IVC* sumBinaryImages(IVC *imagem1, IVC *imagem2) {

	int x, y, i, pos;
	IVC *resultado = vc_image_new(imagem1->width, imagem1->height, 1, 255);

	//Verificar imagens
	if ((imagem1->width != imagem2->width) || (imagem1->height != imagem2->height) || (imagem1->channels != imagem2->channels))
		return NULL;

	for (y = 0; y < imagem1->height; y++)
	{
		for (x = 0; x < imagem1->width; x++)
		{

			pos = y * imagem1->bytesperline + x * imagem1->channels;

			if ( ((int) imagem1->data[pos]) == 255 || ((int) imagem2->data[pos]) == 255)
				resultado->data[pos] = ((unsigned char) 255);
			else
				resultado->data[pos] = ((unsigned char) 0);
		}
	}

	vc_write_image("Resultados/resultadoEmSum.pgm", resultado);

	return resultado;
}

int hue360To255(int hue) {
	return (int) (hue / 360.0f * 255.0f);
}

int hue255To360(int hue) {
	return (int) (hue / 255.0f * 360.0f);
}

int value100To255(int value) {
	return (int)(value / 100.0f * 255.0f);
}

int value255To100(int value) {
	return (int)(value / 255.0f * 100.0f);
}

int countBluePixels(IVC *imagem) {

	IVC *hsvImage = vc_image_copy(imagem);
	int i, x, y, position, contador = 0;
	int hue, sat, value;

	vc_rgb_to_hsv(hsvImage);

	for (y = 0; y < hsvImage->height; y++)
	{
		for (x = 0; x < hsvImage->width; x++)
		{
			position = y * hsvImage->bytesperline + x * hsvImage->channels;

			hue = hsvImage->data[position];
			sat = hsvImage->data[position + 1];
			value = hsvImage->data[position + 2];

			//Caso o pixel seja azul incrementa o contador
			if ((hue >= hue360To255(214) && hue <= hue360To255(269)) && (value >= value100To255(15) && value <= value100To255(60)) && (sat >= value100To255(15) && sat <= value100To255(100))) {
				contador++;
			}
		}
	}

	vc_image_free(hsvImage);
	return contador;

}

int countRedPixels(IVC *imagem) {

	IVC *hsvImage = vc_image_copy(imagem);
	int i, x, y, position, contador = 0;
	int hue, sat, value;

	vc_rgb_to_hsv(hsvImage);

	for (y = 0; y < hsvImage->height; y++)
	{
		for (x = 0; x < hsvImage->width; x++)
		{
			position = y * hsvImage->bytesperline + x * hsvImage->channels;

			hue = hsvImage->data[position];
			sat = hsvImage->data[position + 1];
			value = hsvImage->data[position + 2];

			//Caso o pixel seja azul incrementa o contador
			if ((hue >= hue360To255(350) && hue <= hue360To255(360)) || (hue >= hue360To255(0) && hue <= hue360To255(8))) {

				if ((sat >= value100To255(54) && sat <= value100To255(100)) && (value >= value100To255(31) && value <= value100To255(93)))
					contador++;
			}

		}
	}

	vc_image_free(hsvImage);
	return contador;

}

//Copia duas imagens
IVC *vc_image_copy(IVC *src) {

	int x, y, i;
	IVC *dst = (IVC *)malloc(sizeof(IVC));

	dst->width = src->width;
	dst->height = src->height;
	dst->bytesperline = src->bytesperline;
	dst->channels = src->channels;
	dst->levels = src->levels;
	dst->data = (unsigned char *)malloc(dst->width * dst->height * dst->channels * sizeof(char));

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			for (i = 0; i < src->channels; i++)
			{
				dst->data[(y * src->bytesperline + x * src->channels) + i] = src->data[(y * src->bytesperline + x * src->channels) + i];
			}
		}
	}
	
	return dst;
}

//Blur pela média
int meanBlur(IVC *src, IVC *dst, int kernel) { //Implementa o requisito de remoção de ruido de uma imagem

	int x, y, kx, ky, delta, soma, posK, position, i;
	int channels = src->channels;
	int *somas = (int*)malloc(sizeof(int) * channels);

	delta = (kernel - 1) / 2;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			//Limpar soma do pixel anterior
			for (i = 0; i < channels; i++)
			{
				somas[i] = 0;
			}

			position = y * src->bytesperline + x * src->channels;

			for (ky = -delta; ky <= delta; ky++)
			{
				for (kx = -delta; kx <= delta; kx++)
				{
					if (kx + x >= 0 && ky + y >= 0 && kx + x < src->width && ky + y < src->height) {

						posK = (y + ky) * src->bytesperline + (x + kx) * src->channels;

						for (int i = 0; i < channels; i++)
						{
							somas[i] += src->data[posK + i];
						}
						
					}
				}
			}

			//Faz a média e atribui valor
			for (i = 0; i < channels; i++)
			{
				dst->data[position + i] = somas[i] / (kernel * kernel);
			}

		}
	}

	return 1;
}

//Procura por sinais de transito numa imagem
//Retorna o numero de sinais encontrados
int checkForSignals(IVC *imagem, IVC **sinais) {

	int encontrados = 0;
	return encontrados;
}

//Desenha uma caixa a volta de cada blob
int drawBoundingBox(IVC *imagem, OVC *blobs, int *nlabels, COLOR *color) {

	int x, y, width, height, i, position;
	width = imagem->width;
	height = imagem->height;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			position = y * imagem->bytesperline + x * imagem->channels;

			for (i = 0; i < *nlabels; i++)
			{

				if (!check_blob(blobs[i], imagem))
					continue;

				if ( ((x >= blobs[i].x) && (x < (blobs[i].x + blobs[i].width))) && (y == blobs[i].y || y == blobs[i].y + blobs[i].height)) { //Pinta a parte de cima da caixa
					paintPixel(imagem, position, color);
				}

				if (((y > blobs[i].y) && (y < (blobs[i].y + blobs[i].height))) && (x == blobs[i].x || x == blobs[i].x + blobs[i].width)) { //Pinta a parte esquerda da caixa
					paintPixel(imagem, position, color);
				}


			}
		}
	}

	return 1;
}

//Coloca 1 pixel da cor desejada no centro de cada blob
int drawGravityCentre(IVC *imagem, OVC *blobs, int *nlabels, COLOR *color) {

	int x, y, i, position;

	for (y = 0; y < imagem->height; y++)
	{
		for (x = 0; x < imagem->width; x++)
		{
			position = y * imagem->bytesperline + x * imagem->channels;

			for (i = 0; i < *nlabels; i++)
			{

				if (!check_blob(blobs[i], imagem))
					continue;

				if (x == blobs[i].xc && y == blobs[i].yc)
					paintPixel(imagem, position, color);
			}
		}
	}

	return 1;
}

//Conforme os channels e os levels que a imagem tem, ele pinta de acordo
int paintPixel(IVC *imagem, int position, COLOR *cor) {

	int i;

	if (imagem->channels == 1)
		imagem->data[position] = cor->gray;
	else if (imagem->channels == 3)
	{
		imagem->data[position] = cor->r;
		imagem->data[position + 1] = cor->g;
		imagem->data[position + 2] = cor->b;
	}
	else { //No caso de isto ter mais channels(Improvavel)

		imagem->data[position] = imagem->levels;

		for (i = 1; i < imagem->channels; i++)
		{
			imagem->data[position + i] = 0;
		}
	}

	return 1;
}

//Função para procurar sinal de stop em uma imagem
int temStop() {
	return 0;
}
#pragma endregion


#pragma region MORFOLOGICOS_BINARY


//Dilatação de uma imagem binária
int vc_binary_dilate(IVC *src, IVC *dst, int size) {

	int x, y, xk, yk, pos, w, h, painel, posk;
	h = src->height;
	w = src->width;
	int centro;

	painel = (size - 1) / 2;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			centro = 0;

			for (yk = -painel; yk <= painel; yk++)
			{
				for (xk = -painel; xk < painel; xk++)
				{

					if ((y + yk >= 0) && (y + yk < src->height) && (x + xk >= 0) && (x + xk < src->width))
					{
						posk = (y + yk) * w + (x + xk);

						if (src->data[posk] == 255)
						{
							centro = 255;
							xk = 1000;
							yk = 1000;
						}
					}

				}
			}

			dst->data[pos] = centro;
		}
	}

	return 1;
}

//erosão de uma imagem binária
int vc_binary_erode(IVC *src, IVC *dst, int size) {

	int x, y, xk, yk, pos, w, h, painel, posk;
	h = src->height;
	w = src->width;
	int centro;

	painel = (size - 1) / 2;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			centro = 255;

			for (yk = -painel; yk <= painel; yk++)
			{
				for (xk = -painel; xk < painel; xk++)
				{

					if ((y + yk >= 0) && (y + yk < src->height) && (x + xk >= 0) && (x + xk < src->width))
					{
						posk = (y + yk) * w + (x + xk);

						if (src->data[posk] == 0)
						{
							centro = 0;
							xk = 1000;
							yk = 1000;
						}
					}

				}
			}

			dst->data[pos] = centro;
		}
	}
	return 1;
}

//abertura binaria
int vc_binary_open(IVC *src, IVC *dst, int sizeErode, int sizeDilate) {

	int ret = 1;
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	ret &= vc_binary_erode(src, tmp, sizeErode);
	ret &= vc_binary_dilate(tmp, dst, sizeDilate);

	vc_image_free(tmp);

	return ret;
}

//Fecho binario
int vc_binary_close(IVC *src, IVC *dst, int sizeErode, int sizeDilate) {

	int ret = 1;
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	ret &= vc_binary_dilate(src, tmp, sizeDilate);
	ret &= vc_binary_erode(tmp, dst, sizeErode);

	vc_image_free(tmp);

	return ret;
}

#pragma endregion


#pragma region MORFOLOGICOS_GRAY
//dilatação grayscale
int vc_gray_dilate(IVC *src, IVC *dst, int size) {

	int x, y, xk, yk, pos, w, h, painel, posk;
	h = src->height;
	w = src->width;
	int max;

	painel = (size - 1) / 2;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			max = 0;

			for (yk = -painel; yk <= painel; yk++)
			{
				for (xk = -painel; xk < painel; xk++)
				{

					if ((y + yk >= 0) && (y + yk < src->height) && (x + xk >= 0) && (x + xk < src->width))
					{
						posk = (y + yk) * w + (x + xk);

						if (src->data[posk] > max)
							max = src->data[posk];
					}

				}
			}

			dst->data[pos] = max;
		}
	}

	return 1;
}

//erosao grayscale
int vc_gray_erode(IVC *src, IVC *dst, int size) {

	int x, y, xk, yk, pos, w, h, painel, posk;
	h = src->height;
	w = src->width;
	int min;

	painel = (size - 1) / 2;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			min = src->levels + 1;

			for (yk = -painel; yk <= painel; yk++)
			{
				for (xk = -painel; xk < painel; xk++)
				{

					if ((y + yk >= 0) && (y + yk < src->height) && (x + xk >= 0) && (x + xk < src->width))
					{
						posk = (y + yk) * w + (x + xk);

						if (src->data[posk] < min)
							min = src->data[posk];
					}

				}
			}

			dst->data[pos] = min;
		}
	}

	return 1;
}

//abertura gray
int vc_gray_open(IVC *src, IVC *dst, int sizeErode, int sizeDilate) {

	int ret = 1;
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	ret &= vc_gray_erode(src, tmp, sizeErode);
	ret &= vc_gray_dilate(tmp, dst, sizeDilate);

	vc_image_free(tmp);

	return ret;
}

//Fecho gray
int vc_gray_close(IVC *src, IVC *dst, int sizeErode, int sizeDilate) {

	int ret = 1;
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	ret &= vc_gray_dilate(src, tmp, sizeDilate);
	ret &= vc_gray_erode(tmp, dst, sizeErode);

	vc_image_free(tmp);

	return ret;
}

#pragma endregion


#pragma region GRAY_TO_BINARY

// Calcula o threshold a usar pela media total
int vc_gray_to_binary_mean(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int i, size;
	float media = 0;

	size = width * height * channels;

	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 1) return 0;

	for (i = 0; i < size; i++)
	{
		media += data[i];
	}
	media = (float)media / size;

	for (i = 0; i < size; i++)
	{
		if (data[i] > media) data[i] = 255;
		else data[i] = 0;
	}

	return 1;
}

//Gray to binary midpoint
int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel)
{
	int x, y, z, a;
	int painel, posk;
	int min, max;
	float thershold;

	painel = (kernel - 1) / 2;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			min = src->levels + 1;
			max = -1;

			for (z = -painel; z <= painel; z++)
			{
				for (a = -painel; a <= painel; a++)
				{
					if (y + z >= 0 && x + a >= 0 && y + z < src->height && x + a < src->width)
					{
						posk = (y + z) * src->width + (x + a);

						if (src->data[posk] <= min)
							min = src->data[posk];

						if (src->data[posk] >= max)
							max = src->data[posk];
					}
				}
			}

			thershold = ((float)(min + max) * 0.5f);

			if (src->data[y * src->width + x] > thershold)
			{
				dst->data[y * src->width + x] = 255;
			}
			else
			{
				dst->data[y * src->width + x] = 0;
			}

		}
	}

	return 1;
}

//Gray to binary
int vc_gray_to_binary(IVC *srcdst, int threshold) {

	int i;
	unsigned char *data;
	int width = srcdst->width;
	int height = srcdst->height;
	int channels = srcdst->channels;
	int bytesperline = srcdst->bytesperline;
	int size;

	data = (unsigned char *)srcdst->data;
	size = width * height * channels;

	for (i = 0; i < size; i++)
	{
		if (data[i] < threshold)
			data[i] = 0;
		else
			data[i] = 255;
	}

}

//Gray to binary pelo metodo Bernsen
int vc_gray_to_binary_bernsen(IVC *src, IVC *dst, int kernel) 
{
	unsigned char *data_src = (unsigned char *)src->data, *data_dst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y, xX, yY, vmin = src->levels, vmax = 0, delta, pos, pox;
	float treshold = 0;

	delta = (kernel - 1) / 2;


	//verificar imagem
	if ((width <= 0) || (height <= 0) || (data_src == NULL)) return 0;
	if (channels != 1) return 0;
	if ((width != dst->width) || (height != dst->height) || (dst->data == NULL)) return 0;
	if (dst->channels != 1) return 0;

	for (x = 0; x < width; x++)
	{
		for (y = 0; y < height; y++)
		{
			vmin = src->levels;
			vmax = 0;
			for (xX = -delta; xX <= delta; xX++)
			{
				for (yY = -delta; yY <= delta; yY++)
				{
					if (xX + x >= 0 && yY + y >= 0 && xX + x < width && yY + y < height)
					{
						pox = (yY + y)  * bytesperline + (xX + x) * channels;
						if (data_src[pox] < vmin) vmin = data_src[pox];
						if (data_src[pox] > vmax) vmax = data_src[pox];
					}
				}
			}
			pos = y * bytesperline + x * channels;

			if (vmax - vmin < 15) treshold = src->levels / 2;
			else treshold = (vmin + vmax) / 2;

			if (data_src[pos] > treshold) data_dst[pos] = 255;
			else data_dst[pos] = 0;
		}
	}
}

//Gray to binary pelo metodo adaptativo
int vc_gray_to_binary_adapt(IVC *src, IVC *dst, int kernel)
{
	unsigned char *data_src = (unsigned char *)src->data, *data_dst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y, xX, yY, vmin = src->levels, vmax = 0, delta, pos, pox, soma = 0, n;
	float treshold = 0, media = 0, desvio = 0;

	delta = (kernel - 1) / 2;
	n = kernel * kernel;

	if ((width <= 0) || (height <= 0) || (data_src == NULL)) return 0;
	if (channels != 1) return 0;
	if ((width != dst->width) || (height != dst->height) || (dst->data == NULL)) return 0;
	if (dst->channels != 1) return 0;

	for (x = 0; x < width; x++)
	{
		for (y = 0; y < height; y++)
		{
			for (xX = -delta; xX <= delta; xX++)
			{
				for (yY = -delta; yY <= delta; yY++)
				{
					if (xX + x >= 0 && yY + y >= 0 && xX + x < width && yY + y < height)
					{
						pox = (yY + y)  * bytesperline + (xX + x) * channels;
						soma += data_src[pox];
					}
				}
			}

			media = (float)soma / n;
			soma = 0;

			for (xX = -delta; xX <= delta; xX++)
			{
				for (yY = -delta; yY <= delta; yY++)
				{
					if (xX + x >= 0 && yY + y >= 0 && xX + x < width && yY + y < height)
					{
						pox = (yY + y)  * bytesperline + (xX + x) * channels;
						soma += pow((data_src[pox] - media), 2);
					}
				}
			}
			soma = soma / (n - 1);
			desvio = (float)sqrt(soma);

			treshold = media - 0.2 * desvio;

			pos = y * bytesperline + x * channels;

			if (data_src[pos] > treshold) data_dst[pos] = 255;
			else data_dst[pos] = 0;
		}
	} return 1;
}

#pragma endregion


#pragma region RGB_TO_HSV

//RGB para HSV
int vc_rgb_to_hsv(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, hue, saturation, value;
	float rgb_max, rgb_min;
	int i, size;

	// Verifica��o de erros
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3) return 0;

	size = width * height * channels;

	for (i = 0; i<size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		// Calcula valores m�ximo e m�nimo dos canais de cor R, G e B
		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value == 0.0f)
		{
			hue = 0.0f;
			saturation = 0.0f;
		}
		else
		{
			// Saturation toma valores entre [0,255]
			saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

			if (saturation == 0.0f)
			{
				hue = 0.0f;
			}
			else
			{
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b))
				{
					hue = 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if ((rgb_max == r) && (b > g))
				{
					hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if (rgb_max == g)
				{
					hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
				}
				else /* rgb_max == b*/
				{
					hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
				}
			}
		}

		// Atribui valores entre [0,255]
		data[i] = (unsigned char)(hue / 360.0f * 255.0f);
		data[i + 1] = (unsigned char)(saturation);
		data[i + 2] = (unsigned char)(value);
	}

	return 1;
}

#pragma endregion


#pragma region RGB_GRAY

// Calculado pela formula
int vc_rgb_to_gray(IVC * src, IVC *dst) 
{
	unsigned char *data_src = (unsigned char *)src->data, *data_dst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels, bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels, channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;
	float r, g, b;


	//verificar imagem
	if ((width <= 0) || (height <= 0) || (data_src == NULL)) return 0;
	if (channels_src != 3) return 0;
	if ((width != dst->width) || (height != dst->height) || (data_dst == NULL)) return 0;
	if (channels_dst != 1) return 0;

	//Inverter Imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			r = data_src[pos_src];
			g = data_src[pos_src + 1];
			b = data_src[pos_src + 2];

			data_dst[pos_dst] = (unsigned char)((r *0.299) + (g*0.587) + (b *0.114));

		}
	}
	return 1;
}

// Calculado pela media
int vc_rgb_to_gray_mean(IVC * src, IVC *dst)
{
	unsigned char *data_src = (unsigned char *)src->data, *data_dst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->width * src->channels, bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels, channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;
	int r, g, b;


	//verificar imagem
	if ((width <= 0) || (height <= 0) || (data_src == NULL)) return 0;
	if (channels_src != 3) return 0;
	if ((width != dst->width) || (height != dst->height) || (data_dst == NULL)) return 0;
	if (channels_dst != 1) return 0;

	//Inverter Imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			r = data_src[pos_src];
			g = data_src[pos_src + 1];
			b = data_src[pos_src + 2];

			data_dst[pos_dst] = (unsigned char)((r + g + b) / 3);

		}
	}
	return 1;
}

#pragma endregion


#pragma region EQUALIZAÇÃO

int vc_gray_histogram_equalization(IVC *src, IVC *dst) {

	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	int npixeis = width * height;
	double pdf[256] = { 0.0 };
	double cdf[256] = { 0.0 };
	double cdfmin;
	int hist[256] = { 0 };
	long int pos;
	int x, y, i;
	unsigned char k;

	//Percorre todos os pixeis da imagem
	for (pos = 0; pos < npixeis; pos++)
	{
		//Conta o numero de vezes que cada intensidade de cor ocorr
		hist[src->data[pos]++];
	}


	//Calcula pdf da imagem
	for (i = 0; i < 256; i++)
	{
		pdf[i] = (double)hist[i] / (double)npixeis;
	}

	//Calcula CDF da imagem

	return 1;
}
#pragma endregion


#pragma region FILTROS DOMINIO ESPACIAL


//Deixa passar baixas frequências
int vc_gray_lowpass_mean_filter(IVC *src, IVC *dst, int kernel) {

	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, kx, ky, delta, soma, posK, position, i;
	int channels = src->channels;
	int contador;

	delta = (kernel - 1) / 2;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			soma = 0;
			contador = 0;
			position = y * src->bytesperline + x * src->channels;

			for (ky = -delta; ky <= delta; ky++)
			{
				for (kx = -delta; kx <= delta; kx++)
				{
					if (kx + x >= 0 && ky + y >= 0 && kx + x < src->width && ky + y < src->height) {

						posK = (y + ky) * src->bytesperline + (x + kx) * src->channels;
						soma += src->data[posK];
						contador++;

					}
				}
			}


			dst->data[position] = soma / contador;

		}
	}

	return 1;
}

int vc_gray_lowpass_median_filter(IVC *src, IVC *dst, int kernel) {

	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, kx, ky, delta, posK, position, i;
	int channels = src->channels;
	int contador;
	int *valoresKernel = (int*)malloc(sizeof(int) * (kernel * kernel));

	delta = (kernel - 1) / 2;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			contador = 0;
			position = y * src->bytesperline + x * src->channels;

			for (ky = -delta; ky <= delta; ky++)
			{
				for (kx = -delta; kx <= delta; kx++)
				{
					if (kx + x >= 0 && ky + y >= 0 && kx + x < src->width && ky + y < src->height) {

						posK = (y + ky) * src->bytesperline + (x + kx) * src->channels;
						valoresKernel[contador] = (int)src->data[posK];
						contador++;

					}
				}
			}

			ordena_array_asc(valoresKernel, contador);
			dst->data[position] = (unsigned char)get_median(valoresKernel, contador);

		}
	}

	free(valoresKernel);

	return 1;
}

int vc_gray_lowpass_gaussian_filter(IVC *src, IVC *dst) {
	return 1;
}

int vc_gray_highpass_filter(IVC *src, IVC *dst, int kernel) {

	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, kx, ky, delta, posK, position, i;
	int channels = src->channels;
	int contador;
	int *valoresKernel = (int*)malloc(sizeof(int) * (kernel * kernel));
	int posA, posB, posC, posD, posX, posE, posF, posG, posH, soma;

	delta = (kernel - 1) / 2;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			contador = 0;
			position = y * src->bytesperline + x * src->channels;

			for (ky = -delta; ky <= delta; ky++)
			{
				for (kx = -delta; kx <= delta; kx++)
				{
					if (kx + x >= 0 && ky + y >= 0 && kx + x < src->width && ky + y < src->height) {

						posK = (y + ky) * src->bytesperline + (x + kx) * src->channels;
						contador++;

					}
				}
			}

			//dst->data[position] = ;

		}
	}

	free(valoresKernel);

	return 1;
}

//Retorna o valor da mediana de um array já ordenado
int get_median(int *array, int tamanho) {

	boolean isEven = false;
	int position = tamanho / 2;

	if (tamanho % 2 == 0)
		isEven = true;

	if (isEven == true)
	{
		return (array[position] + array[position - 1]) / 2;
	}
	else {
		return array[position];
	}

	return 0;
		
}
#pragma endregion


#pragma region DOMINIO FREQUENCIAS

#pragma endregion


#pragma region OUTROS
//Ordena um array por ordem ascendente
int ordena_array_asc(int *array, int tamanho) {

	int i, j, tmp;

	for (j = 0; j < tamanho - 1; j++)
	{
		for (i = j + 1; i < tamanho; i++)
		{
			if (array[j] > array[i])
			{
				tmp = array[j];
				array[j] = array[i];
				array[i] = tmp;
			}
		}
	}

	return 1;
}

void MyLog(LogType tipo, char *mensagem) {

	if (tipo == ERROR)
		printf("[ERRO]: %s \n", mensagem);

	if (tipo == SUCCESS)
		printf("[SUCESSO]: %s \n", mensagem);

	if (tipo == WARNING)
		printf("[AVISO]: %s \n", mensagem);

	if (tipo == INFO)
		printf("[INFO]: %s \n", mensagem);

	return;
}

COLOR* newColor(int r, int g, int b) {

	COLOR *cor = (COLOR*)malloc(sizeof(COLOR));
	cor->r = (unsigned char)r;
	cor->g = (unsigned char)g;
	cor->b = (unsigned char)b;

	cor->gray = (unsigned char)((r *0.299) + (g*0.587) + (b *0.114));

	return cor;

}

int vc_gray_negative(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	//verificar imagem
	if ((width <= 0) || (height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 1) return 0;

	//Inverter Imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 255 - data[pos];
		}
	}
	return 1;
}

int vc_rgb_negative(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	//verificar imagem
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3) return 0;

	//Inverter Imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 255 - data[pos];
			data[pos + 1] = 255 - data[pos + 1];
			data[pos + 2] = 255 - data[pos + 2];
		}
	}
	return 1;
}

int vc_red_filter(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	//verificar imagem
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3) return 0;

	//Inverter Imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos + 1] = 0;
			data[pos + 2] = 0;
		}
	}
	return 1;
}

int vc_remove_red(IVC * srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	//verificar imagem
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3) return 0;

	//Inverter Imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 0;
		}
	}
	return 1;
}

#pragma endregion

//FIM DE FUNÇÔES FEITAS NA AULA