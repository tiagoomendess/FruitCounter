#include <stdio.h>
#include <opencv\cv.h>
#include <opencv\cxcore.h>
#include <opencv\highgui.h>

#include <string.h>

#include "vc.h"
#include "labelling.h"
#include "ca.h"

int main(void)
{
	// Vídeo
	char *videofile = "video-tp2.avi";
	CvCapture *capture;
	IplImage *frame = NULL;

	//Variaveis trabalho
	IVC *ivcFrame;
	IVC *hsvFrame;
	IVC *binaryFrame;
	IVC *labels;
	OVC *blobsAtual;
	OVC *blobsAnterior;
	int nlabels = 0; 
	int nlabelsOld = 0;
	int frameJump = 10; //Quantos frames deverão ser ignorados apos ser analizado 1
	int countJump = 0;
	bool firstFrame = true;
	Fruta frutas[10];
	int nFrutas = 0;

	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;

	// Texto
	CvFont font, fontbkg;
	double hScale = 0.5;
	double vScale = 0.5;
	int lineWidth = 1;
	char str[500] = { 0 };

	// Outros
	int key = 0;

	/* Leitura de vídeo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video-tp2.avi deverá estar localizado no mesmo directório que o ficheiro de código fonte.
	*/
	capture = cvCaptureFromFile(videofile);

	/* Verifica se foi possível abrir o ficheiro de vídeo */
	if (!capture)
	{
		fprintf(stderr, "Erro ao abrir o ficheiro de vídeo!\n");
		return 1;
	}

	/* Número total de frames no vídeo */
	video.ntotalframes = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);
	/* Frame rate do vídeo */
	video.fps = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
	/* Resolução do vídeo */
	video.width = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
	video.height = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);

	//Alocar memória para as imagens IVC
	MyLog(INFO, "A alocar memoria para imagens");
	ivcFrame = vc_image_new(video.width, video.height, 3, 255);
	labels = vc_image_new(video.width, video.height, 1, 255);
	hsvFrame = vc_image_new(video.width, video.height, 3, 255);
	binaryFrame = vc_image_new(video.width, video.height, 1, 255);

	/* Cria uma janela para exibir o vídeo */
	cvNamedWindow("VC - TP2", CV_WINDOW_AUTOSIZE);

	/* Inicializa a fonte */
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, hScale, vScale, 0, lineWidth);
	cvInitFont(&fontbkg, CV_FONT_HERSHEY_SIMPLEX, hScale, vScale, 0, lineWidth + 1);

	MyLog(INFO, "A analisar video. Pressione P para pausa ou Q para sair.");

	while (key != 'q') {

		if (key == 'p') {

			key = 'a';
			while (key != 'p') {
				key = cvWaitKey();
			}
		}

		/* Leitura de uma frame do vídeo */
		frame = cvQueryFrame(capture);

		/* Verifica se conseguiu ler a frame */
		if (!frame) break;

		/* Número da frame a processar */
		video.nframe = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES);

		//Converter frame Ipl para IVC
		IplImage_to_IVC(frame, ivcFrame);

		//Descartar frames caso necessário
		if (countJump == frameJump) //Vai analizar o frame
		{
			countJump = 0;

			if (!firstFrame) {

				blobsAnterior = vc_copy_blobs(blobsAtual, nlabels);
				nlabelsOld = nlabels;
			}
			else
			{
				blobsAnterior = NULL;
			}

			//Passar frame em RGB para HSV
			vc_copy_image_data(ivcFrame, hsvFrame);
			vc_rgb_to_hsv(hsvFrame);

			//Passar frame do video para binario
			frame_to_binary(ivcFrame, hsvFrame, binaryFrame);

			blobsAtual = vc_binary_blob_labelling(binaryFrame, labels, &nlabels);
			vc_binary_blob_info(labels, blobsAtual, nlabels);

			if (nlabels > 0) {

				analisa_blobs(hsvFrame, binaryFrame, blobsAtual, nlabels, blobsAnterior, nlabelsOld, frutas, &nFrutas);
			}


		}
		else //Não vai analizar, "dropa" o frame
		{
			countJump++;
		}

		//Desenhar caixas delimitadoras
		if (nlabels > 0) {

			drawBoundingBox(ivcFrame, blobsAtual, &nlabels, newColor(0, 255, 255));
			drawGravityCentre(ivcFrame, blobsAtual, &nlabels, newColor(0, 255, 255));
		}
			

		//Converter de IVC para IPL
		IVC_to_IplImage(ivcFrame, frame);

		/* Exemplo de inserção texto na frame */
		sprintf(str, "RESOLUCAO: %dx%d", video.width, video.height);
		cvPutText(frame, str, cvPoint(20, 20), &fontbkg, cvScalar(0, 0, 0));
		cvPutText(frame, str, cvPoint(20, 20), &font, cvScalar(255, 255, 255));
		sprintf(str, "TOTAL DE FRAMES: %d", video.ntotalframes);
		cvPutText(frame, str, cvPoint(20, 40), &fontbkg, cvScalar(0, 0, 0));
		cvPutText(frame, str, cvPoint(20, 40), &font, cvScalar(255, 255, 255));
		sprintf(str, "FRAME RATE: %d", video.fps);
		cvPutText(frame, str, cvPoint(20, 60), &fontbkg, cvScalar(0, 0, 0));
		cvPutText(frame, str, cvPoint(20, 60), &font, cvScalar(255, 255, 255));
		sprintf(str, "N. FRAME: %d", video.nframe);
		cvPutText(frame, str, cvPoint(20, 80), &fontbkg, cvScalar(0, 0, 0));
		cvPutText(frame, str, cvPoint(20, 80), &font, cvScalar(255, 255, 255)); 

		sprintf(str, "Qtd. FRUTA: %d", nFrutas);
		cvPutText(frame, str, cvPoint(20, 100), &fontbkg, cvScalar(0, 0, 0));
		cvPutText(frame, str, cvPoint(20, 100), &font, cvScalar(255, 255, 255));

		//esvaziar memoria
		//free(blobs);

		/* Exibe a frame */
		cvShowImage("VC - TP2", frame);

		/* Sai da aplicação, se o utilizador premir a tecla 'q' */
		
		firstFrame = false;
		key = cvWaitKey(1);
		
	}

	imprime_relatorio(frutas, nFrutas);

	/* Fecha a janela */
	cvDestroyWindow("VC - TP2");

	/* Fecha o ficheiro de vídeo */
	cvReleaseCapture(&capture);

	vc_image_free(ivcFrame);
	vc_image_free(labels);

	getchar();

	return 0;
}