
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/ml.hpp"


using namespace std;
using namespace cv;
using namespace cv::ml;

//MultipleImageWindow* miw;
Mat light_pattern;
//Ptr<SVM> svm;
Ptr<SVM> svm = ml::SVM::create();
Scalar green(0,255,0), blue (255,0,0), red (0,0,255);

static Scalar randomColor( RNG& rng )
{
int icolor = (unsigned) rng;
return Scalar( icolor&255, (icolor>>8)&255, (icolor>>16)&255 );
}


/**
* Extract the features for all objects in one image
* 
* @param Mat img input image
* @param vector<int> left output of left coordinates for each object
* @param vector<int> top output of top coordintates for each object
* @return vector< vector<float> > a matrix of rows of features for each object detected
**/
vector< vector<float> > ExtractFeatures(Mat img, vector<int>* left=NULL, vector<int>* top=NULL)
{
  vector< vector<float> > output;
  vector<vector<Point> > contours;
  Mat input= img.clone();
  
  vector<Vec4i> hierarchy;
  findContours(input, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
  // Check the number of objects detected
  if(contours.size() == 0 ){
    return output;
  }
  RNG rng( 0xFFFFFFFF );
  for(int i=0; i<contours.size(); i++){
    
    Mat mask= Mat::zeros(img.rows, img.cols, CV_8UC1);
    drawContours(mask, contours, i, Scalar(1), FILLED, LINE_8, hierarchy, 1);
    Scalar area_s= sum(mask);
    float area= area_s[0];
    
    if(area>500){ //if the area is greather than min.

      RotatedRect r= minAreaRect(contours[i]);
      float width= r.size.width;
      float height= r.size.height;
      float ar=(width<height)?height/width:width/height;

      vector<float> row;
      row.push_back(area);
      row.push_back(ar);
      output.push_back(row);
      if(left!=NULL){
          left->push_back((int)r.center.x);
      }
      if(top!=NULL){
          top->push_back((int)r.center.y);
      }
      
      //miw->addImage("Extract Features", mask*255);
      //miw->render();
      //waitKey(10);
    }
  }
  return output;
}

Mat removeLight(Mat img, Mat pattern)
{
  Mat aux;
  
  // Require change our image to 32 float for division
  Mat img32, pattern32;
  cout << "X";
  img.convertTo(img32, CV_32F);
  pattern.convertTo(pattern32, CV_32F);
  // Divide the imabe by the pattern
  cout << "channels of img32: " << img32.channels() << endl;
  cout << "channels of pattern32: " << pattern32.channels() << endl;

  if (img32.channels() != 1) {
    cv::cvtColor(img32, img32, cv::COLOR_BGR2GRAY); //aang    
  }

  
  aux= 1-(img32/pattern32);
  
  // Scale it to convert o 8bit format
  aux=aux*255;
  // Convert 8 bits format
  aux.convertTo(aux, CV_8U);

  //equalizeHist( aux, aux );
  return aux;
}


/**
* Preprocess an input image to extract components and stats
* @params Mat input image to preprocess
* @return Mat binary image
*/
Mat preprocessImage(Mat input)
{

    Mat result;
  // Remove noise
  Mat img_noise, img_box_smooth;
  medianBlur(input, img_noise, 3);
 
  //Apply the light pattern
  Mat img_no_light;
  img_noise.copyTo(img_no_light);

  img_no_light= removeLight(img_noise, light_pattern);  

  // Binarize image for segment
  threshold(img_no_light, result, 30, 255, THRESH_BINARY);

  return result;
}


/**
* Read all images in a folder creating the train and test vectors
* @param folder string name
* @param label assigned to train and test data
* @param number of images used for test and evaluate algorithm error
* @param trainingData vector where store all features for training
* @param reponsesData vector where store all labels corresopinding for training data, in this case the label values
* @param testData vector where store all features for test, this vector as the num_for_test size
* @param testResponsesData vector where store all labels corresponiding for test, has the num_for_test size with label values
* @return true if can read the folder images, false in error case
**/
bool readFolderAndExtractFeatures(string folder, int label, int num_for_test, 
  vector<float> &trainingData, vector<int> &responsesData,  
  vector<float> &testData, vector<float> &testResponsesData)
{
  VideoCapture images;
  if(images.open(folder)==false){
    cout << "Can not open the folder images" << endl;
    return false;
  }
  Mat frame;
  int img_index=0;
  cout << "oHERE";
  while( images.read(frame) ){
    //// Preprocess image
      
      Mat pre= preprocessImage(frame);
      cout << "HERE";
    // Extract features
    vector< vector<float> > features= ExtractFeatures(pre);
    for(int i=0; i< features.size(); i++){
      if(img_index >= num_for_test){
        trainingData.push_back(features[i][0]);
        trainingData.push_back(features[i][1]);
        responsesData.push_back(label);    
      }else{
        testData.push_back(features[i][0]);
        testData.push_back(features[i][1]);
        testResponsesData.push_back((float)label);    
      }
    }
    img_index++;
  }
  return true;  
}

void trainAndTest()
{
  vector< float > trainingData;
  vector< int > responsesData;
  vector< float > testData;
  vector< float > testResponsesData;

  int num_for_test= 20;
  cout << "YEAH";
  // Get the nut images
  readFolderAndExtractFeatures("../../aoi/data/nut/tuerca_%04d.pgm", 0, num_for_test, trainingData, responsesData, testData, testResponsesData);
  // Get and process the ring images
  readFolderAndExtractFeatures("../../aoi/data/ring/arandela_%04d.pgm", 1, num_for_test, trainingData, responsesData, testData, testResponsesData);
  // get and process the screw images
  readFolderAndExtractFeatures("../../aoi/data/screw/tornillo_%04d.pgm", 2, num_for_test, trainingData, responsesData, testData, testResponsesData);


  
  cout << "Num of train samples: " << responsesData.size() << endl;

  cout << "Num of test samples: " << testResponsesData.size() << endl;


  // Merge all data 
  Mat trainingDataMat(trainingData.size()/2, 2, CV_32FC1, &trainingData[0]);

  Mat responses(responsesData.size(), 1, CV_32SC1, &responsesData[0]);
  Mat testDataMat(testData.size()/2, 2, CV_32FC1, &testData[0]);

  Mat testResponses(testResponsesData.size(), 1, CV_32FC1, &testResponsesData[0]);

  cout << "DDD" <<endl;
  // Set up SVM's parameters
  //SVM::Params params;
  //params.svmType    = SVM::C_SVC;
  //params.kernelType = SVM::CHI2;
  //params.termCrit   = TermCriteria(TermCriteria::MAX_ITER, 100, 1e-6);
  Ptr<ml::TrainData> tData = ml::TrainData::create(trainingDataMat, ROW_SAMPLE, responses);

  svm->setType(ml::SVM::C_SVC);
  svm->setKernel(SVM::CHI2);
  //  svm->setGamma(3);
  svm->setTermCriteria(TermCriteria(TermCriteria::MAX_ITER, 100, 1e-6));

  //svm = StatModel::train<SVM>(trainingDataMat, ROW_SAMPLE, responses, params);  
  // Train the SVM
  svm->train(tData);

    
  if(testResponsesData.size()>0){
    cout << "Evaluation" << endl;
    cout << "==========" << endl;
    // Test the ML Model
    Mat testPredict;
    svm->predict(testDataMat, testPredict);
    cout << "Prediction Done" << endl;
    // Error calculation
    Mat errorMat= testPredict!=testResponses;
    float error= 100.0f * countNonZero(errorMat) / testResponsesData.size();
    cout << "Error: " << error << "\%" << endl;
    // Plot training data with error label
    //plotTrainData(trainingDataMat, responses, &error);

  }else{
      //plotTrainData(trainingDataMat, responses);
  }
  
}


void startAOI() {

    String img_file = "../../aoi/data/test.pgm";
    String light_pattern_file = "../../aoi/data/pattern.pgm";


     // Load input image to process the test
    Mat img= imread(img_file, 0);
    if(img.data==NULL){
	cout << "Error loading image "<< img_file << endl;
	return;
    }
    Mat img_output= img.clone();
    cvtColor(img_output, img_output, COLOR_GRAY2BGR);
    
    //Load image pattern to process 
    light_pattern= imread(light_pattern_file, 0);
    if(light_pattern.data==NULL){
	// Calculate light pattern
	cout << "ERROR: Not light patter loaded" << endl;
	return;
    }

    medianBlur(light_pattern, light_pattern, 3);
    trainAndTest();

 //// Preprocess image
  Mat pre= preprocessImage(img);
  ////End preprocess
    // Extract features
  vector<int> pos_top, pos_left;
  vector< vector<float> > features= ExtractFeatures(pre, &pos_left, &pos_top);

  cout << "Num objects extracted features " << features.size() << endl;

  for(uint i=0; i< features.size(); i++){
      
    cout << "Data Area AR: " << features[i][0] << " " << features[i][1] << endl;
    
    Mat trainingDataMat(1, 2, CV_32FC1, &features[i][0]);
    cout << "Features to predict: " << trainingDataMat << endl;
    float result= svm->predict(trainingDataMat);
    cout << result << endl;
    
    
    stringstream ss;
    Scalar color;
    if(result==0){
      color= green; // NUT
      ss << "NUT";
    }
    else if(result==1){
      color= blue; // RING
      ss << "RING" ;
    }
    else if(result==2){
      color= red; // SCREW
      ss << "SCREW";
    }
        
    putText(img_output, 
      ss.str(), 
      Point2d(pos_left[i], pos_top[i]), 
      FONT_HERSHEY_SIMPLEX, 
      0.4, 
	    color);
  }

  // Show images
  //miw->addImage("Binary image", pre);
  //miw->addImage("Result", img_output);
  //miw->render();
  namedWindow("Result", WINDOW_NORMAL);
  imshow("Result", img_output);
  
  waitKey(0);
    
}
