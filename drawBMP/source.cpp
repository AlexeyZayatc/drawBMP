#include <iostream>
#include <bit>
#include <fstream>
#include <filesystem>
#include <exception>
#include <Windows.h>

class BmpImage {
public:
	BmpImage() {}
	BmpImage(const std::string& path) {
		openBMP(path);
	}

	void openBMP(const std::string& path) {
		if (!std::filesystem::exists(path)) {
			throw std::exception(std::string("File does not exist at path: " + path).c_str());
		}
		bmpFile.open(path,std::ios_base::in | std::ios_base::binary);
		if (bmpFile) {
			bmpFile.read(std::bit_cast<char*,BITMAPFILEHEADER*>(&fileHeader), sizeof(fileHeader));
			if (fileHeader.bfType != 0x4D42) {
				throw std::exception("Header is not 4D42");
			}		

			bmpFile.read(std::bit_cast<char*, BITMAPINFOHEADER*>(&infoHeader), sizeof(infoHeader));			
			if (infoHeader.biBitCount == 32) {
				if (infoHeader.biSize >= (sizeof(infoHeader) + sizeof(BITMAPCOLORHEADER))) {
					bmpFile.read(std::bit_cast<char*, BITMAPCOLORHEADER*>(&colorHeader), sizeof(colorHeader));
					checkColorHeader(colorHeader);
				}
				else {
                     std::cerr << "Warning! The file does not seem to contain bit mask information\n";
                     throw std::runtime_error("Unrecognized file format.");
				}
			}

			bmpFile.seekg(fileHeader.bfOffBits,bmpFile.beg);
			data.resize(((infoHeader.biWidth * infoHeader.biBitCount+31)/32)*4*infoHeader.biHeight);
			if (infoHeader.biWidth % 4 == 0) {
				bmpFile.read(std::bit_cast<char*, uint8_t*>(data.data()), data.size());
			}
			else {
				rowStride = infoHeader.biWidth * infoHeader.biBitCount / 8;
				uint32_t newStride = makeStrideAligned(4);
				std::vector<uint8_t> paddingRow(newStride - rowStride);

				for (size_t y = 0; y < infoHeader.biHeight; ++y) {
					bmpFile.read(std::bit_cast<char*, uint8_t*>(data.data() + rowStride * y), rowStride);
					bmpFile.read(std::bit_cast<char*, uint8_t*>(paddingRow.data()), paddingRow.size());
				}
			}
		}
		closeBMP();
	}

	void displayBMP() {
		int channels = infoHeader.biBitCount / 8;
		for (long i = infoHeader.biHeight-1; i >=0 ; i--) {
			for (size_t j = 0; j < infoHeader.biWidth; j++) {
				if (data[channels * (i * infoHeader.biWidth + j) + 0] == 255 &&
					data[channels * (i * infoHeader.biWidth + j) + 1] == 255 &&
					data[channels * (i * infoHeader.biWidth + j) + 2] == 255 
					) {
					std::cout << "B";
				}
				else {
					std::cout << " ";
				}
			}
			std::cout << "\n";
		}
	}

	void closeBMP() {
		bmpFile.close();
	}
		
	~BmpImage() {
		closeBMP();
	}
private:

	struct BITMAPCOLORHEADER {
		uint32_t red_mask{ 0x00ff0000 };         // Bit mask for the red channel
		uint32_t green_mask{ 0x0000ff00 };       // Bit mask for the green channel
		uint32_t blue_mask{ 0x000000ff };        // Bit mask for the blue channel
		uint32_t alpha_mask{ 0xff000000 };       // Bit mask for the alpha channel
		uint32_t color_space_type{ 0x73524742 }; // Default "sRGB" (0x73524742)
		uint32_t unused[16]{ 0 };                // Unused data for sRGB color space
	};
	
	std::ifstream bmpFile;

	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	BITMAPCOLORHEADER colorHeader;

	std::vector<uint8_t> data;
	uint32_t rowStride;

	void checkColorHeader(BITMAPCOLORHEADER& bmp_color_header) {
		BITMAPCOLORHEADER expected_color_header;
		if (expected_color_header.red_mask != bmp_color_header.red_mask ||
		expected_color_header.blue_mask != bmp_color_header.blue_mask ||
		expected_color_header.green_mask != bmp_color_header.green_mask ||
		expected_color_header.alpha_mask != bmp_color_header.alpha_mask) {
			throw std::runtime_error("Unexpected color mask format! The program expects the pixel data to be in the BGRA format");
		}
		if (expected_color_header.color_space_type != bmp_color_header.color_space_type) {
			throw std::runtime_error("Unexpected color space type! The program expects sRGB values");		
		}	
	}
    uint32_t makeStrideAligned(uint32_t align_stride) {
		uint32_t new_stride = rowStride;
		while (new_stride % align_stride != 0) {
			    new_stride++;
		}
		return new_stride;
   }
};

int main(int argc, char* argv[]) {
	if (argc == 1) {
		std::cout << "Please enter path to file!\n";
		return 0;
	}
	else if (argc > 2) {
		std::cout << "Please enter only one path to file.\n";
		return 0;
	}
	try{
		BmpImage img(argv[1]);
		img.displayBMP();
	}
	catch (std::exception e) {
		std::cerr << e.what() << "\n";
	}

	return 0;
}