// ForecroundProcessor.cpp

#include "ForegroundProcessor.h"
//#include "stdafx.h"

namespace ForegroundProcessing
{

	////////////////// Foreground Segmentation //////////////////////
	void ForegroundProcessor::segmentForeground(std::list<Frame> & oldFrames, Frame & frame)
	{
		switch(algorithm)
		{
		case 0:
			segmentForegroundFast(frame, threshval, iterations);
			break;
		case 1:
			segmentForegroundArea(frame, threshval, iterations, minArea, minQuotient);
			break;
		case 2:
			segmentForegroundShadow(oldFrames, frame, threshval, iterations, minArea, minQuotient);
		case 3:
			segmentForegroundSlow(frame, threshval, minDist);
			break;
		}
	}

	void ForegroundProcessor::segmentForegroundFast(Frame & frame, int threshval, int iterations)
	{
		threshMap(frame.probMap, threshval); //Threshold at threshval
	
		// Erode followed by 3 iterations of dilate (3x3 kernel)
		openingBinMap(frame.probMap, iterations); 
	
		getObjects(frame);
		return;
	}

	void ForegroundProcessor::segmentForegroundSlow(Frame & frame, int threshval, double minDist)
	{
		threshMap(frame.probMap, threshval); //Threshold at threshval
	
		getObjectsDistMap(frame, minDist);
	
		return;
	}

	void ForegroundProcessor::segmentForegroundArea(Frame & frame, int threshval, int Iterations, double minArea, double minQuotient)
	{
		threshMap(frame.probMap, threshval); //Threshold at threshval
	
		closingBinMap(frame.probMap, iterations); 

		getObjectsArea(frame, minArea, minQuotient);
	
		return;
	}


	void ForegroundProcessor::segmentForegroundShadow(std::list<Frame> & oldFrames, Frame & frame, int threshval, int iterations, double minQuotient, double minArea)
	{
		threshMap(frame.probMap, threshval);

		dilateBinMap(frame.probMap);
		//closingBinMap(frame.probMap, iterations);

		suppressShadows(oldFrames, frame, minArea, minQuotient);
		cout << "DERPADERPA 1337" << endl;
		Mat herp = frame.probMap;
		imshow( "ASDFASF", herp );

		//openingBinMap(frame.probMap, iterations);
		dilateBinMap(frame.probMap, 3);
		erodeBinMap(frame.probMap, 5);
		dilateBinMap(frame.probMap, 4);
		getObjectsArea(frame, minArea, minQuotient);
				
		//herp.convertTo(herp, CV_32FC1, 1, 0);
		

		return;

	}


	void ForegroundProcessor::init(int threshval, int iterations, double minDist, double minArea, double minQuotient)
	{
		this->algorithm = algorithm;
		this->threshval = threshval;
		this->iterations = iterations;
		this->minDist = minDist;
		this->minArea = minArea; 
		this->minQuotient =	minQuotient;
		this->frameCounter = 0;
	}

	////////////////// Private Functions //////////////////////
	///////////////////////////////////////////////////////////
	
	////////////////// Object Detection ///////////////////////
	void ForegroundProcessor::getObjects(Frame & frame)
	{
		vector<vector<Point>> contours;
		findContours( frame.probMap.clone(), contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	
		for(unsigned int i = 0; i < contours.size(); i++)
		{		
			//Create an object for every countour using the boundingRect command
			frame.objects.push_back(Object(boundingRect(contours[i])));	
		}
	}

	void ForegroundProcessor::getObjectsArea(Frame & frame, double minArea, double minQuotient)
	{
		double objArea;
		Rect objRect;
		vector<vector<Point>> contours;
		findContours( frame.probMap.clone(), contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	
		for(unsigned int i = 0; i < contours.size(); i++)
		{		
			objArea = contourArea(contours[i]);
			objRect = boundingRect(contours[i]);
			
			if ((objArea > minArea) && (objArea > (objRect.width * objRect.height)/minQuotient))
			{
				//Create an object for every countour using the boundingRect command
				frame.objects.push_back(Object(objRect));	
			}
			
			//
			//frame.objects.push_back(Object(boundingRect(contours[i])));	
		}
	}

	void ForegroundProcessor::suppressShadows(std::list<Frame> & oldFrames, Frame & frame, double minArea, double minDist)
	{
		if (shadowModel.empty())
			shadowModel =  Mat::zeros(frame.image.size(), CV_8UC3);
		frameCounter++;
		if (frameCounter < 10)
		{
			shadowModel = (frame.image / 9);
			cout << "AAAAHHH: " << frameCounter << endl;
			return;
		}
		int i = 11;
		if (frameCounter == 10)
		{
			//blur( current->image, blurredImage, Size( i, i ), Point(-1,-1) );
			GaussianBlur( shadowModel, shadowModel, Size( i, i ), 0,0 );
			
		}

		vector<vector<Point>> contours;
		findContours( frame.probMap.clone(), contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		Vec3f newColorVec, oldColorVec, tempVec;
		Mat lastImage = oldFrames.front().image.clone();
		GaussianBlur( lastImage, lastImage, Size( i, i ), 0,0 );
		
		double objArea;
		Rect objRect;
		double dist = 0;
		
		for(unsigned int i = 0; i < contours.size(); i++)
		{	
			objArea = contourArea(contours[i]);
			objRect = boundingRect(contours[i]);
			vector<Point> contour = contours[i];
			//cout << "total cols: "<< frame.probMap.size().width << "total rows: "<< frame.probMap.size().height  << endl;
			//cout << "max X "<< objRect.x + objRect.width - 1 << "max Y: "<< objRect.y + objRect.height - 1 << endl;
			//check the interior of all contours to see wich pixels have a uniform decrease over all color channels.
			for( int j = objRect.x; j < objRect.x + objRect.width; j++)
				{ for( int k = objRect.y; k < objRect.y + objRect.height; k++) 
					{ 
						//If object is not outside the contour
						if (pointPolygonTest(contour, Point(j, k), false) >= 0) 
						{
							Point matPos(j,k);
							//cout << "DERPADERPA 1" << endl;
							normalize((Vec3f)lastImage.at<Vec3b>(matPos), newColorVec, 1, NORM_L2); // (float)norm((Vec3f)frame.image.at<uchar>(j,k));
							//cout << "DERPADERPA 1" << endl;
							normalize((Vec3f)shadowModel.at<Vec3b>(matPos), oldColorVec, 1, NORM_L2);
							//cout << "DERPADERPA 2" << endl;
							//cout << "DERPADERPA 1" << endl;
							//cout << norm(newColorVec) << endl;
							//cout << newColorVec << endl;
							//cout << newColorVec.dot(oldColorVec) << endl;
							//cout << "total rows: "<< frame.probMap.size().height << "total cols: "<< frame.probMap.size().width << endl;
							//cout << norm(shadows.at<Vec3f>(j,k)) << " row: "<< j << " col: "<< k << endl;
							if (newColorVec.dot(oldColorVec) > 0.995) // Parallell color vectors
							{
								
								frame.probMap.at<uchar>(matPos) = 0;
								/*
								//cout << "DERPADERPA 2" << endl;
								oldColorVec = (Vec3f)lastImage.at<uchar>(matPos);
								//cout << "DERPADERPA 3" << endl;
								oldColorVec = oldColorVec / (float)norm(oldColorVec);
								cout << newColorVec - oldColorVec << endl;
								//tempVec = newColorVec.cross(tempVec);
								//cout << "DERPADERPA 5" << endl;
								//SHADOW DETECTED
								if ( newColorVec.dot(oldColorVec) > 0.99) 
								{
									//cout << "DERPADERPA 6" << endl;
									shadows.at<Vec3f>(matPos) = oldColorVec;
									//cout << "DERPADERPA 7" << endl;
									frame.probMap.at<uchar>(matPos) = 128;
									//cout << "DERPADERPA 5" << endl;
								}
							} 
							else
							{
								//cout << "DERPADERPA 8" << endl;
								tempVec = shadows.at<Vec3f>(matPos);
								//cout << "DERPADERPA 9" << endl;
								//tempVec = newColorVec.cross(tempVec);
								//cvCrossProduct(&newColorVec, &tempVec, &tempVec);
								//cout << "DERPADERPA 10" << endl;
								if ( newColorVec.dot(tempVec) > 0.99 )
								{
									//cout << "DERPADERPA 12" << endl;
									frame.probMap.at<uchar>(matPos) = 128;
								}*/
													
							}
						} 
					}		
				}
		}
	}
	
	
	
	void ForegroundProcessor::getObjectsDistMap(Frame & frame, double minDist)
	{
		vector<vector<Point>> contours;
		findContours( frame.probMap.clone(), contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		Rect objRect;
		double dist = 0;
		double minSize = 20;
		for(unsigned int i = 0; i < contours.size(); i++)
		{	
			objRect = boundingRect(contours[i]);
			vector<Point> contour = contours[i];
		
			//Measure distance to the contour of all pixels within the bounding box.
			for( int j = objRect.x; j < objRect.x + objRect.width; j++)
				{ for( int k = objRect.y; k < objRect.y + objRect.height; k++) 
					{ 
						if (pointPolygonTest(contour, Point(j, k), false) == 1) //If object is inside the contour
						{
							dist = max(dist, pointPolygonTest(contour, Point(j, k), true)); // Calculate distance
						} 
					}		
				}
			if (dist > minDist) //Create object only if distance is great enough.
			{		
				frame.objects.push_back(Object(objRect));	
			}
			dist = 0;
		}
	}

	////////////////// Image Processing //////////////////////
	void ForegroundProcessor::threshMap(Mat probMap, int threshval)
	{
		int maxVal = 255;
		threshold(probMap, probMap, threshval, maxVal, THRESH_BINARY);
	}

	void ForegroundProcessor::openingBinMap(Mat probMap, int iterations)
	{
		cv::Mat kernel;
		kernel = getStructuringElement( MORPH_RECT, Size(3, 3));
		erode(probMap, probMap, kernel, cv::Point(-1,-1), iterations);
		dilate(probMap, probMap, kernel, cv::Point(-1,-1), iterations);
	}

	void ForegroundProcessor::closingBinMap(Mat probMap, int iterations)
	{
		cv::Mat kernel;
		kernel = getStructuringElement( MORPH_RECT, Size(3, 3));
		dilate(probMap, probMap, kernel, cv::Point(-1,-1), iterations);
		erode(probMap, probMap, kernel, cv::Point(-1,-1), iterations);
	}

	void ForegroundProcessor::erodeBinMap(Mat probMap, int iterations)
	{
		cv::Mat kernel;
		kernel = getStructuringElement( MORPH_RECT, Size(3, 3));
		erode(probMap, probMap, kernel, cv::Point(-1,-1), iterations);
	}

	void ForegroundProcessor::dilateBinMap(Mat probMap, int iterations)
	{
		cv::Mat kernel;
		kernel = getStructuringElement( MORPH_RECT, Size(3, 3));
		dilate(probMap, probMap, kernel, cv::Point(-1,-1), iterations);
	}

}
