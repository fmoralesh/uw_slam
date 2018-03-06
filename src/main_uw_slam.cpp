/**
* This file is part of UW-SLAM.
* 
* Copyright 2018.
* Developed by Fabio Morales,
* Email: fabmoraleshidalgo@gmail.com; GitHub: @fmoralesh
*
* UW-SLAM is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* UW-SLAM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with UW-SLAM. If not, see <http://www.gnu.org/licenses/>.
*/

#include <args.hxx>
#include <thread>
#include <System.h>
#include <Tracker.h>

#include <thread>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "Eigen/Core"


// C++ namespaces
using namespace uw;
using namespace cv;
using namespace std;
using namespace cv::cuda;

int start_index;
string images_path;
string calibration_path;
string ground_truth_path;
cuda::DeviceInfo device_info;

void ShowSettings() {
    cout << "CUDA enabled devices detected: " << device_info.name() << endl;
    cout << "Directory of calibration xml file: " << calibration_path << endl;
    cout << "Directory of images: " << images_path  << endl;
    if (not (ground_truth_path == ""))
        cout << "Directory of ground truth poses: " << ground_truth_path << endl;
    cout << endl;
}

int main (int argc, char *argv[]) {

    cout << "===================================================" << endl;
    int n_cuda_devices = cuda::getCudaEnabledDeviceCount();
    if (n_cuda_devices > 0) {
        cuda::setDevice(0);
    } else {
        cout << "No CUDA device detected" << endl;
        cout << "Exiting..." << endl;
        return -1;
    }

    // Parse section
    try {
        parser.ParseCLI(argc, argv);
    } 
    catch (args::Help) {
        cout << parser;
        return 0;
    } 
    catch (args::ParseError e) {
        cerr << e.what() << endl;
        cerr << parser;
        return 1;
    } catch (args::ValidationError e) {
        cerr <<  e.what() << endl;
        cerr << parser;
        return 1;
    }
    if (!dir_dataset) {
        cout<< "Introduce path of images as argument." << endl;
        cerr << "Use -h, --help command to see usage." << endl;
        return 1;
    } else {
        images_path = args::get(dir_dataset);
    }
    if (dir_groundtruth) {
        ground_truth_path = args::get(dir_groundtruth);
    } else {
        ground_truth_path = "";  // Need to change for final release
    }
    if (parse_calibration) {
        calibration_path = args::get(parse_calibration);
    } else {
        calibration_path = "/home/fabiomorales/catkin_ws/src/uw-slam/sample/calibration.xml";  // Need to change for final release
    }
    if (start_i) {
        start_index = args::get(start_i);
    } else {
        start_index = 0;
    }
    
    // Show parser settings and CUDA information
    ShowSettings();

    // Create new System
    System* uwSystem = new System(argc, argv, start_index);

    // Calibrates system with certain Camera Model (currently only RadTan) 
    uwSystem->Calibration(calibration_path);
    
    // Add list of the dataset images names (optionally ground truth reference)
    uwSystem->AddListImages(images_path, ground_truth_path);
    
    // Start SLAM process
    // Read images one by one from directory provided
    for (int i=start_index; i<uwSystem->images_list_.size(); i++) {
        if (not uwSystem->initialized_) {
            uwSystem->InitializeSystem();
            uwSystem->AddFrame(i);
            uwSystem->AddKeyFrame(i);
        } else {
            uwSystem->AddFrame(i);
            uwSystem->Tracking();
            uwSystem->visualizer_->UpdateMessages(uwSystem->previous_frame_);
        }

        // Delete oldest frame
        if (uwSystem->num_frames_> 10) {
            uwSystem->FreeFrames();
        }
    }
    // Delete system
    uwSystem->~System();
    return 0;
}