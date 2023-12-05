#include <opencv2/opencv.hpp>
#include <argparse/argparse.hpp>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

// used to determine if we need to calculate the angle later........
static std::string referenceImagePath;

/////////////////////////////////////////////////////////////////////////////////
/// Quck app to rotate an image by an angle, optionally try to detect how far 
/// off it is from level ( needs an image with close to horizonal lines to work)
/////////////////////////////////////////////////////////////////////////////////


/**
 * check if a file is an image based on its extension
 *
 * @param path The file path.
 * @return bool True if the file is an image, false otherwise.
 */
bool isImageFile(const std::filesystem::path& path) {
	// @todo check what opencv actually supports..
	const std::vector<std::string> imageExtensions = { ".jpg", ".jpeg", ".png", ".bmp", ".tif", ".tiff" };
	return std::find(imageExtensions.begin(), imageExtensions.end(), path.extension()) != imageExtensions.end();
}


/**
 * automatically try to determine the rotation angle of an image.
 *
 * @param src The source image.
 * @return double The estimated rotation angle in degrees.
 */
double determineRotationAngle(const cv::Mat& src) {

	// convert to grayscale
	cv::Mat gray;
	cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
	cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);

	// edge detection
	cv::Mat edges;
	cv::Canny(gray, edges, 50, 150, 3);

	// line detection
	std::vector<cv::Vec4i> lines;
	cv::HoughLinesP(edges, lines, 1, CV_PI / 180, 100, 50, 10);

	// calculate the average angle of the lines
	// not a great method
	double angle = 0.0;
	int count = 0;
	for (const auto& line : lines) {
		double theta = atan2(line[3] - line[1], line[2] - line[0]);
		angle += theta;
		count++;
	}
	if (count > 0) {
		angle /= count; // average the angle
	}

	return angle * (180.0 / CV_PI);
}


/**
 * calculate the rotation angle from a reference image.
 *
 * @param referenceImagePath Path to the reference image.
 * @param successful Reference to a boolean flag indicating success or failure.
 * @param verbose Flag to enable verbose output.
 * @return double The calculated rotation angle.
 */
double calculateReferenceAngle(const std::string& referenceImagePath, bool& successful, bool verbose) {
	cv::Mat referenceImage = cv::imread(referenceImagePath, cv::IMREAD_COLOR);
	if (referenceImage.empty()) {
		std::cerr << "Could not open or find the reference image: " << referenceImagePath << std::endl;
		successful = false;
		return 0.0;
	}

	double angle = determineRotationAngle(referenceImage);
	if (verbose) {
		std::cout << "Rotation angle determined from reference image: " << angle << " degrees" << std::endl;
	}
	successful = true;
	return angle;
}

/**
 * rotate an image by a given angle
 *
 * @param src The source image to be rotated.
 * @param angle The angle in degrees to rotate the image.
 * @return cv::Mat The rotated image.
 */
cv::Mat rotateImage(const cv::Mat& src, double angle) {
	cv::Point2f center(src.cols / 2.0f, src.rows / 2.0f);
	cv::Mat rot = cv::getRotationMatrix2D(center, angle, 1.0);
	cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), src.size(), (float)angle).boundingRect2f();
	rot.at<double>(0, 2) += bbox.width / 2.0 - src.cols / 2.0;
	rot.at<double>(1, 2) += bbox.height / 2.0 - src.rows / 2.0;

	cv::Mat dst;
	cv::warpAffine(src, dst, rot, bbox.size());
	return dst;
}


/**
 * process a single image file - rotate and save it, if the angle isn't 0.0
 *
 * @param inputFile The path of the input image file.
 * @param outputFile The path where the output image will be saved.
 * @param angle The angle to rotate the image.
 * @param verbose Flag to enable verbose output.
 * @return bool Status code (true for success, false for error).
 */
bool processSingleImage(const std::string& inputFile, const std::string& outputFile, double angle, bool verbose) {

	if (angle == 0.0) {
		return false;
	}
	cv::Mat image = cv::imread(inputFile, cv::IMREAD_COLOR);
	if (image.empty()) {
		std::cerr << "Could not open or find the image: " << inputFile << std::endl;
		return false;
	}
	cv::Mat rotatedImage = rotateImage(image, angle);
	if (rotatedImage.empty()) {
		std::cerr << "Error rotating the image." << std::endl;
		return false;
	}

	if (!cv::imwrite(outputFile, rotatedImage)) {
		std::cerr << "Failed to write the image to: " << outputFile << std::endl;
		return false;
	}

	if (verbose) {
		std::cout << "Image rotated successfully and saved to " << outputFile << std::endl;
	}
	return true;
}

/**
 * process all images in a directory. recursively as an option.
 *
 * @param inputDir The input directory path.
 * @param outputDir The output directory path.
 * @param angle The angle to rotate the images.
 * @param recursive Flag to enable recursive processing of subdirectories.
 * @param verbose Flag to enable verbose output.
 * @return bool Status code (true for success, false for error).
 */
bool processDirectory(const std::string& inputDir, const std::string& outputDir, double angle, bool recursive, bool verbose) {
	std::filesystem::directory_iterator iter(inputDir), end;
	std::error_code ec;

	while (iter != end) {
		const auto& entry = *iter;
		if (isImageFile(entry.path())) {
			std::string outputFilePath = (std::filesystem::path(outputDir) / entry.path().filename()).string();


			if (referenceImagePath.empty()) {
				bool successful;

				double t_angle = calculateReferenceAngle(entry.path().string(), successful, verbose);
				if (successful) {
					angle = t_angle;
				}


			}

			if (!processSingleImage(entry.path().string(), outputFilePath, angle, verbose) ) {
				std::cerr << "Failed to process image: " << entry.path() << std::endl;
				return false;

			}
			else if (verbose) {
				std::cout << "Processed " << entry.path() << std::endl;
			}
		}
		if (recursive && entry.is_directory()) {
			processDirectory(entry.path().string(), outputDir, angle, true, verbose);
		}

		iter.increment(ec);
		if (ec && verbose) {
			std::cerr << "Warning: Error accessing " << iter->path() << ": " << ec.message() << std::endl;

		}
	}
	return true;
}


int main(int argc, char** argv) {
	argparse::ArgumentParser program("rotImage");

	program.add_argument("-i", "--input")
		.help("Specify the input image file path or input directory path.")
		.required();

	program.add_argument("-o", "--output")
		.help("Specify the output image file path or output directory path.")
		.required();

	program.add_argument("-a", "--angle")
		.help("Specify the rotation angle in degrees.")
		.scan<'g', double>()
		.default_value(0.0);

	program.add_argument("-r", "--recursive")
		.default_value(false)
		.implicit_value(true)
		.help("Recursively process all image files in subdirectories.");

	program.add_argument("-v", "--verbose")
		.default_value(false)
		.implicit_value(true)
		.help("Enable verbose output.");

	program.add_argument("-d", "--detect")
		.default_value(false)
		.implicit_value(true)
		.help("Automatically detect and correct the rotation angle of the image.");

	program.add_argument("-ref", "--reference")
		.help("Specify the path to a reference image. The rotation angle of this image will be used for all other images.");

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		std::cerr << "Error parsing arguments: " << err.what() << std::endl;
		std::cerr << program;
		return 1;
	}

	std::string inputPath = program.get<std::string>("input");
	std::string outputPath = program.get<std::string>("output");
	double angle = program.get<double>("angle");
	bool recursive = program["--recursive"] == true;
	bool verbose = program["--verbose"] == true;
	
	if (program.is_used("--reference")) {
		referenceImagePath = program.get<std::string>("--reference");
	}
	else {
		referenceImagePath = "";
	}

	// use the reference image to calculate the angle to rotate by
	if (!referenceImagePath.empty()) {
		bool successful;

		double t_angle = calculateReferenceAngle(referenceImagePath, successful, verbose);
		if (successful) {
			angle = t_angle;
		}
	}

	if (std::filesystem::is_directory(inputPath)) {
		if (!std::filesystem::exists(outputPath)) {
			if (!std::filesystem::create_directories(outputPath)) {
				std::cerr << "Failed to create output directory: " << outputPath << std::endl;
				return 1;
			}
		}
		return processDirectory(inputPath, outputPath, angle, recursive, verbose);
	}
	else {

		// if no reference image, then try to calculate the angle to rotate by, this is only going to work on 
		// very specific images.
		// we're not calculating angle, so a 0.0 is ok
		if (( angle == 0.0 ) && referenceImagePath.empty()) {

			bool successful;
			angle = calculateReferenceAngle(inputPath, successful, verbose);
			if (!successful) return 1;

		}

		return processSingleImage(inputPath, outputPath, angle, verbose);
	}
}
