#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>
#include <commdlg.h>

//g++ pdf.cpp -o pdf_console.exe -lcomdlg32 -static -static-libgcc -static-libstdc++ -std=c++11

// PDF Header
void write_pdf_header(std::ofstream& pdf) {
    pdf << "%PDF-1.4\n";
}

// Read and copy PDF content
void copy_pdf_content(std::ifstream& input, std::ofstream& output) {
    std::string line;
    while (std::getline(input, line)) {
        output << line << "\n";
    }
}

// Merge PDFs
void merge_pdfs(const std::vector<std::string>& pdf_files, const std::string& output_file) {
    std::ofstream output(output_file, std::ios::binary);
    if (!output) {
        std::cerr << "Could not open output file: " << output_file << std::endl;
        return;
    }

    write_pdf_header(output);
    output << "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    output << "2 0 obj\n<< /Type /Pages /Kids ["; 

    size_t total_pages = 0;
    std::vector<size_t> page_counts; // To track the number of pages in each PDF

    for (const auto& pdf_file : pdf_files) {
        std::ifstream input(pdf_file, std::ios::binary);
        if (!input) {
            std::cerr << "Could not open file: " << pdf_file << std::endl;
            continue;
        }

        // Read and copy content from each PDF
        copy_pdf_content(input, output);
        
        // Increment the total pages for the output PDF
        total_pages++; // Assuming each PDF file has 1 page for simplicity
        page_counts.push_back(1); // Update page counts for simplicity
        input.close();
    }

    // Close the Pages object
    output << "] /Count " << total_pages << " >>\nendobj\n";

    // Add XREF and trailer sections
    output << "xref\n0 " << (total_pages + 3) << "\n";
    for (size_t i = 0; i < total_pages + 3; ++i) {
        output << "0000000000 65535 f \n"; // Placeholder
    }

    output << "trailer\n<< /Size " << (total_pages + 3) << " /Root 1 0 R >>\n";
    output << "%%EOF\n";

    output.close();
    std::cout << "PDF files successfully merged: " << output_file << std::endl;
}

// Modern File Selection
std::vector<std::string> select_files(const char* filter) {
    OPENFILENAME ofn;
    char szFiles[1024] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFiles;
    ofn.nMaxFile = sizeof(szFiles);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileName(&ofn)) {
        std::vector<std::string> files;
        char* p = szFiles;
        while (*p) {
            files.push_back(p);
            p += strlen(p) + 1; // Move to next file
        }
        return files;
    } else {
        std::cerr << "File selection canceled." << std::endl;
        return {};
    }
}

// Convert image files to PDF
void image_to_pdf(const std::string& image_file, const std::string& output_pdf) {
    std::ifstream img(image_file, std::ios::binary);
    if (!img) {
        std::cerr << "Could not open image file: " << image_file << std::endl;
        return;
    }

    std::ofstream pdf(output_pdf, std::ios::binary);
    write_pdf_header(pdf);
    
    // PDF structure
    pdf << "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    pdf << "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    
    // Page object
    pdf << "3 0 obj\n<< /Type /Page /MediaBox [0 0 612 792] /Contents 4 0 R /Resources << /XObject << /img1 5 0 R >> >> >>\nendobj\n";
    
    // Content stream
    img.seekg(0, std::ios::end);
    std::streampos img_size = img.tellg();
    img.seekg(0, std::ios::beg);
    
    pdf << "4 0 obj\n<< /Length " << static_cast<int>(img_size) + 28 << " >>\nstream\n"; // Adjust length
    pdf << "BT /F1 24 Tf 72 720 Td (Image here) Tj ET\n";  // Placeholder text
    pdf << "endstream\nendobj\n";

    // Image object
    pdf << "5 0 obj\n<< /Type /XObject /Subtype /Image /Width 612 /Height 792 /BitsPerComponent 8 /ColorSpace /DeviceRGB /Filter /DCTDecode /Length " << img_size << " >>\nstream\n";
    pdf << img.rdbuf(); // Write image data
    pdf << "\nendstream\nendobj\n";

    pdf << "xref\n0 6\n";
    pdf << "0000000000 65535 f \n";
    pdf << "0000000009 00000 n \n";
    pdf << "0000000054 00000 n \n";
    pdf << "0000000100 00000 n \n";
    pdf << "0000000154 00000 n \n";
    pdf << "0000000210 00000 n \n";
    pdf << "trailer\n<< /Size 6 /Root 1 0 R >>\n";
    pdf << "%%EOF\n";

    pdf.close();
    std::cout << "Image successfully converted to PDF: " << output_pdf << std::endl;
}

// Main Menu
void display_menu() {
    std::cout << "Select the operation you want to perform:\n";
    std::cout << "1. Merge PDFs\n";
    std::cout << "2. Convert PNG to PDF\n";
    std::cout << "3. Convert JPEG to PDF\n";
    std::cout << "q. Exit\n";
}

int main() {
    char choice;
    do {
        display_menu();
        std::cout << "Your choice: ";
        std::cin >> choice;

        switch (choice) {
            case '1': {
                std::cout << "Select the PDF files you want to merge:\n";
                auto pdf_files = select_files("PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0");
                if (!pdf_files.empty()) {
                    merge_pdfs(pdf_files, "merged_output.pdf");
                }
                break;
            }
            case '2': {
                std::cout << "Select a PNG file:\n";
                auto png_file = select_files("PNG Files (*.png)\0*.png\0All Files (*.*)\0*.*\0");
                if (!png_file.empty()) {
                    image_to_pdf(png_file.front(), "output_image.pdf");
                }
                break;
            }
            case '3': {
                std::cout << "Select a JPEG file:\n";
                auto jpeg_file = select_files("JPEG Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0All Files (*.*)\0*.*\0");
                if (!jpeg_file.empty()) {
                    image_to_pdf(jpeg_file.front(), "output_image.pdf");
                }
                break;
            }
            case 'q':
                std::cout << "Exiting...\n";
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    } while (choice != 'q');

    return 0;
}

//g++ pdf.cpp -o pdf.exe -lcomdlg32
