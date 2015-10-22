﻿#pragma once
#include "stdafx.h"
#include "FeatureExtraction.h"
#include "MachineLearning.h"

Mat ExtractSURFDescriptor(string path)
{
	Mat pImage = imread(path.c_str());

	SurfFeatureDetector detector;
	vector<KeyPoint> keypoints;
	detector.detect(pImage, keypoints);
	
	// computing descriptors
	Mat descriptors;
	SurfDescriptorExtractor extractor;
	extractor.compute(pImage, keypoints, descriptors);

	return descriptors;
}

Mat ExtractSIFTDescriptor(string path)
{
	Mat pImage = imread(path.c_str());

	SiftFeatureDetector detector;
	vector<KeyPoint> keypoints;
	detector.detect(pImage, keypoints);

	// computing descriptors
	Mat descriptors;
	SiftDescriptorExtractor extractor;
	extractor.compute(pImage, keypoints, descriptors);

	return descriptors;
}

int numClassVideo = 0;
void CreateVocaburary(BOWImgDescriptorExtractor &bowDE, int dictionarySize)
{
	//---dictionary size = number of cluster's centroids
	TermCriteria tc(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 10, 0.01);
	int retries = 1;
	int flags = KMEANS_PP_CENTERS;

	//Get a large set of images to be training set
	string trainpath = "Data/Key frames/";
	vector<string> _listClass = ReadFileList(trainpath);
	numClassVideo = _listClass.size();

	int numClass = (int)_listClass.size();
	BOWKMeansTrainer bowTrainer(dictionarySize, tc, retries, flags);
	for (int classIndex = 0; classIndex < numClass; classIndex++)
	{
		string path = trainpath + _listClass[classIndex] + "/";
		vector<string> _listStringName = ReadFileList(path.c_str());
		Shuffle(&_listStringName[0], _listStringName.size());

		int numFileName = 150;
		for (int j = 0; j < numFileName; j++)
		{
			string filename = path + _listStringName[j];

			cout << filename << endl;

			//Mat descriptors = ExtractSURFDescriptor(filename);
			Mat descriptors = ExtractSIFTDescriptor(filename);

			if (!descriptors.empty())
			{
				bowTrainer.add(descriptors);
			}
			else
			{
				cout << "This image doesn't contain any feature point" << endl;
			}
		}	
	}
	ClusterFeature(bowTrainer, bowDE, "");
}

void ClusterFeature(BOWKMeansTrainer bowTrainer, BOWImgDescriptorExtractor &bowDE, string dictionaryName)
{
	vector<Mat> descriptors = bowTrainer.getDescriptors();
	int count = 0;
	for (vector<Mat>::iterator iter = descriptors.begin(); iter != descriptors.end(); iter++)
	{
		count += iter->rows;
	}
	cout << "Clustering " << count << " features" << endl;
	Mat dictionary = bowTrainer.cluster();
	bowDE.setVocabulary(dictionary);

	string filename = "Data/BOW_dictionary.xml";
	FileStorage fs(filename, FileStorage::WRITE);
	fs << "dictionary" << dictionary;
}

Mat LoadBOWDictionaryFromFile(string filename)
{
	Mat dictionary;

	// read
	FileStorage fs(filename, FileStorage::READ);
	fs["dictionary"] >> dictionary;

	fs.release();

	return dictionary;
}

void CreateBOWTrainingSet(int dictionarySize, SiftFeatureDetector detector, BOWImgDescriptorExtractor bowDE)
{
	//Get a large set of image to be training set
	string trainpath = "Data/Key frames/";

	vector<string> _listClass = ReadFileList(trainpath);
	numClassVideo = _listClass.size();

	Mat bowDescriptor;

	string vocabularyPath = "Data/BOW_dictionary.xml";
	bowDE.setVocabulary(LoadBOWDictionaryFromFile(vocabularyPath));

	Mat trainingData(0, dictionarySize, CV_32FC1);
	Mat trainingLabel(0, 1, CV_32SC1);

	for (int classIndex = 0; classIndex < numClassVideo; classIndex++)
	{
		string path = trainpath + _listClass[classIndex] + "/";
		vector<string> _listStringName = ReadFileList(path.c_str());

		cout << "There are " << _listStringName.size() << " files in folder " << _listClass[classIndex] << endl;

		int numFileName = (int)_listStringName.size();
		for (int filenameIndex = 0; filenameIndex < numFileName; filenameIndex++)
		{
			string filename = path + _listStringName[filenameIndex];

			cout << filename << endl;

			Mat testImage = imread(filename);

			vector<KeyPoint> keypoint;
			detector.detect(testImage, keypoint);

			if (keypoint.size() > 0)
			{
				bowDE.compute(testImage, keypoint, bowDescriptor);

				trainingData.push_back(bowDescriptor);

				int shotID = IdentifyShotFromKeyFrame(_listStringName[filenameIndex]);
				trainingLabel.push_back(shotID);
			}
			else
			{
				cout << "No keypoint can be found in this image. Ignore!" << endl;
			}
		}

		cout << "There are " << trainingData.size() << " sample images with " << trainingLabel.size() << " classes." << endl;

		//Write training data to file for future use	
		string pathTrainData = "Data/BOW_data/" + _listClass[classIndex] + ".xml";
		FileStorage fs(pathTrainData, FileStorage::WRITE);
		fs << "data_matrix" << trainingData;
		fs << "data_label" << trainingLabel;

		trainingData.release();

		trainingLabel.release();
	}

	////Encoding class name values
	//string pathMapData = "Data/ANN_map.txt";
	//map<float, float> mapClassEncode = ClassEncoding(pathMapData, trainingLabel);
	//cout << "Finish encoding class name " << endl;

	//Initialize ANN for training
	//string pathANNModel = "Data/ANN_Model.txt";
	//CreateANNTrainingModel(pathANNModel, dictionarySize, trainingData, trainingLabel, numClassVideo);
	//cout << "Finish creating ANN training model"<< endl;
}

void CreateMPEGTrainingSet()
{
	//Get a large set of image to be training set
	string trainpath = "Data/Key frames/";

	vector<string> _listClass = ReadFileList(trainpath);
	numClassVideo = _listClass.size();

	Mat trainingData(0, 1, CV_32FC1);
	Mat trainingLabel(0, 1, CV_32SC1);

	for (int classIndex = 0; classIndex < numClassVideo; classIndex++)
	{
		string path = trainpath + _listClass[classIndex] + "/";
		vector<string> _listStringName = ReadFileList(path.c_str());

		cout << "There are " << _listStringName.size() << " files in folder " << _listClass[classIndex] << endl;

		int numFileName = (int)_listStringName.size();
		for (int filenameIndex = 0; filenameIndex < numFileName; filenameIndex++)
		{
			string filename = path + _listStringName[filenameIndex];

			cout << filename << endl;

			Mat testImage  = imread(filename);
			Mat descriptor = ExtractMPEGFeature(testImage);

			trainingData.push_back(descriptor);

			int shotID = IdentifyShotFromKeyFrame(_listStringName[filenameIndex]);
			trainingLabel.push_back(shotID);
		}

		cout << "There are " << trainingData.size() << " sample images with " << trainingLabel.size() << " classes." << endl;

		//Write training data to file for future use	
		string pathTrainData = "Data/BOW_data/" + _listClass[classIndex] + ".xml";
		FileStorage fs(pathTrainData, FileStorage::WRITE);
		fs << "data_matrix" << trainingData;
		fs << "data_label" << trainingLabel;

		trainingData.release();

		trainingLabel.release();
	}
}

Mat ExtractBOWFeature(BOWImgDescriptorExtractor bowDE, SiftFeatureDetector detector, Mat image)
{
	Mat bowDescriptor;
	vector<KeyPoint> keypoint;
	detector.detect(image, keypoint);
	bowDE.compute(image, keypoint, bowDescriptor);

	return bowDescriptor;
}

Mat ExtractMPEGFeature(Mat pImage)
{
	vector<float> featureVector;

	Feature *featureExtractor = new Feature();
	Frame *frame = new Frame(pImage);

	//Extract Color Layout Descriptor
	int numYCoef = 10;
	int numCCoef = 6;
	ColorLayoutDescriptor *layout = featureExtractor->getColorLayoutD(frame, numYCoef, numCCoef);
	for (int i = 0; i < numYCoef; i++)
	{
		featureVector.push_back((float)layout->m_y_coeff[i]);
	}
	for (int i = 0; i < numCCoef; i++)
	{
		featureVector.push_back((float)layout->m_cb_coeff[i]);
		featureVector.push_back((float)layout->m_cr_coeff[i]);
	}
	float magnitude = GetMagnitude(featureVector);
	for (int i = 0; i < featureVector.size(); i++)
	{
		if (magnitude>0)
			featureVector[i] /= magnitude;
		else
			featureVector[i] = 0;
	}
	delete layout;

	//Extract Edge Histogram Descriptor
	EdgeHistogramDescriptor *edge = featureExtractor->getEdgeHistogramD(frame);
	for (int i = 0; i < edge->GetSize(); i++)
	{
		featureVector.push_back(edge->GetEdgeHistogramD()[i]);
	}
	delete edge;

	delete frame;

	delete featureExtractor;

	return ToMat(featureVector);
}

bool LoadBOWTrainingSet(string path, Mat &training_data, Mat &training_label)
{
	// read:
	FileStorage fs(path, FileStorage::READ);

	if (fs.isOpened())
	{
		fs["data_matrix"] >> training_data;
		fs["data_label"] >> training_label;
		fs.release();
		return true;
	}
	else
	{
		return false;
	}
}

//---------BASIC IMAGE PROCESSING-----------//

Mat EdgeDetection(Mat img)
{
	Mat edgeImg; 
	cvtColor(img, edgeImg, CV_BGR2GRAY);
	Sobel(edgeImg, edgeImg, CV_8U, 1, 1);

	return edgeImg;
}

int CountEdgePixel(Mat imgEdge)
{
	int count = 0;
	int imageSize = imgEdge.rows * imgEdge.cols;
	for (int i = 0; i < imageSize; i++)
	{
		if (imgEdge.at<uchar>(i) > 0)
		{
			count++;
		}
	}

	return count;
}

double CalculateEdgeMatchingRate(Mat imgEdge1, Mat imgEdge2)
{
	double matchingRate = 0;

	//Count number of matched edge pixel
	int countMatchPixel = 0;
	int imageSize = imgEdge1.rows * imgEdge1.cols;
	for (int i = 0; i < imageSize; i++)
	{
		uchar pixel1 = imgEdge1.at<uchar>(i);
		uchar pixel2 = imgEdge2.at<uchar>(i);
		if (pixel1 == pixel2)
		{
			countMatchPixel++;
		}
	}

	//int edgePixel = MAX( CountEdgePixel(imgEdge1) , CountEdgePixel(imgEdge2) );
	//if (edgePixel > 0)
	//{
		matchingRate = (double)countMatchPixel / (double)imageSize;
	//}

	return matchingRate;
}

KeyFrameDescriptor CalcMPEGDescriptor(Mat img)
{
	KeyFrameDescriptor descriptor;

	Feature *featureExtractor = new Feature();

	Frame *frame = new Frame(img);

	descriptor.colorDesc = featureExtractor->getColorStructureD(frame, 64);
	descriptor.edgeDesc = featureExtractor->getEdgeHistogramD(frame);

	Mat grayImg(img.rows, img.cols, CV_8U);
	Frame *grayFrame = new Frame(img.cols, img.rows);
	cvtColor(img, grayImg, CV_BGR2GRAY);
	grayFrame->setGray(grayImg);
	descriptor.textureDesc = featureExtractor->getHomogeneousTextureD(grayFrame);
	delete grayFrame;

	delete frame;

	delete featureExtractor;

	return descriptor;
}

vector<float> GetMomentDescriptor(Mat image)
{
	Mat normImage;
	image.convertTo(normImage, CV_32FC3);

	vector<float> featureVector;

	//Get a list of Y values from all pixels
	vector<float> listYValues, listCrValues, listCbValues;
	for (int i = 0; i < normImage.rows; i++)
	{
		for (int j = 0; j < normImage.cols; j++)
		{
			Vec3f value = normImage.at<Vec3f>(i, j);
			listYValues.push_back(value.val[0]);
			listCrValues.push_back(value.val[1]);
			listCbValues.push_back(value.val[2]);
			//			cout << value.val[0] << "	" << value.val[1] << "	" << value.val[2] << endl
		}
	}

	float meanY = GetMean<float>(listYValues);
	float sdY = GetStandardDeviation<float>(listYValues, meanY);
	float skewY = GetSkewness<float>(listYValues, meanY);
	//	float kurtosisY = GetKurtosis<float>(listYValues, meanY);

	//	float meanCr	 = GetMean<float>(listCrValues);
	//	float sdCr		 = GetStandardDeviation<float>(listCrValues, meanCr);
	//	float skewCr	 = GetSkewness<float>(listCrValues, meanCr);
	//	float kurtosisCr = GetKurtosis<float>(listCrValues, meanCr);

	//	float meanCb	 = GetMean<float>(listCbValues);
	//	float sdCb		 = GetStandardDeviation<float>(listCbValues, meanCb);
	//	float skewCb	 = GetSkewness<float>(listCbValues, meanCb);
	//	float kurtosisCb = GetKurtosis<float>(listCbValues, meanCb);

	featureVector.push_back(meanY);
	featureVector.push_back(sdY);
	featureVector.push_back(skewY);
	//	featureVector.push_back(kurtosisY);

	//	featureVector.push_back(meanCb);
	//	featureVector.push_back(sdCb);
	//	featureVector.push_back(skewCb);
	//	featureVector.push_back(kurtosisCb);

	//	featureVector.push_back(meanCr);
	//	featureVector.push_back(sdCr);
	//	featureVector.push_back(skewCr);
	//	featureVector.push_back(kurtosisCr);

	NormalizeFeatureVector(featureVector);

	return featureVector;
}