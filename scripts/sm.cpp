// Call from root folder: tmp/sm -t scripts/nice.template -d src

#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace fs=std::filesystem;

enum errors { success=0, no_args, no_template, no_dir, parse_error};

void error(std::string text, int code) {
    std::cerr << text << std::endl;
    exit(code);
}

// Bizzare that we should need this!
void to_upper(std::string& s) {
   for (auto p = s.begin();
        p != s.end(); ++p) {
      *p = toupper(*p);
   }
}

std::string extract(const fs::path& path, const std::string& fname, const std::string &tag) {
    
    // Output stream.
    std::stringstream os;

    // Begin and end tag.
    std::string btag="//{{BEGIN." + tag + "}}";
    std::string etag="//{{END." + tag + "}}";

    fs::path pfull=path / fname;
    std::ifstream fle(pfull);
    std::string line;
    bool inside=false;

    while(std::getline(fle, line))
    {
        // sm include directive?
        if (line.rfind(btag, 0) == 0) 
            inside=true;
        else if (line.rfind(etag, 0) == 0)
            inside=false;
        else if (inside)
            os << line << std::endl; // Just dump it.
    }

    fle.close();
    return os.str();
}

std::string evaluate(const fs::path& root, std::string line, int n) {

    // Output stream.
    std::stringstream os;

    // Is it shorter then 18 characters?
    if (line.size()<18)
        error("Invalid directive.", parse_error);

    // Tag at 11.
    std::string tag = line.substr(11,3); 
    // Cardinality at 15.
    std::string card = line.substr(15,1);
    // From 17 to }} is folder.
    std::string path = line.substr(17, line.rfind("}}")-17);
    if (path=="}" || path==".") path=std::string(); // Empty? i.e no space before }} or .}}

    // Now iterate through all files and extract by tags...
    fs::path p = path.empty() ? root : root / path;
    if (card=="N") {
        // Iterate folders.
        bool first=true;
        for (const auto& e : fs::directory_iterator(p)) {
            const auto name = e.path().filename().string();
            std::string uname=name; to_upper(uname);
            if (first) { 
                os << "#if (__" << uname << "__)" << std::endl;
                first=false;
            } else
                os << "#elif (__" << uname << "__)" << std::endl;
            if (e.is_directory()) {
                fs::path pfull=p / name;
                for (const auto& f : fs::directory_iterator(pfull)) {
                    const auto fname = f.path().filename().string();
                    if (f.is_regular_file()) {
                        os << extract(pfull, fname, tag);
                    }
                } 
            }
        } 
        os << "#endif";
    } else if (card=="1") {
        // Iterate files.
        for (const auto& f : fs::directory_iterator(p)) {
            const auto fname = f.path().filename().string();
            if (f.is_regular_file()) {
                os << extract(p, fname, tag);
            }
        } 
    } else
        error("Invalid cardinality. Only 1 or N are allowed.", parse_error);

    return os.str();
}

// usage: smerge -t <template> -d <root directory> [-r]
int main(int argc, char *argv[]) {

    // Too few args?
    if (argc <= 1)
        error("Usage: sm -t <template> -d <root directory> [-r]", no_args);

    // Directory, template and recurse args.
    std::string d, t;
    bool r=false; 

    // Parse args..
    int i=1;
    while (i<argc) {
        std::string arg=argv[i];
        if (arg=="-t") { // Template. Requires template argument.
            if (i+1==argc)
                error("Expected template path after -t.", no_template);
            t=argv[++i];
        } else if (arg=="-d") {
            if (i+1==argc)
                error("Expected directory path after -d.", no_dir);
            d=argv[++i];
        } else if (arg=="-r") {
            r=true;
        }
        ++i;
    }

    // if we are missing something, use defaults.
    auto directory = d.empty() ? 
        fs::current_path() / "src": 
        fs::path(d);
    auto tplfname = t.empty() ?
        fs::current_path() / "scripts/nice.template":
        fs::path(t);

    // Now let's open the template file and iterate through it, line by line.
    std::ifstream tpls(tplfname);
    std::string line;
    int n=1;

    while(std::getline(tpls, line))
    {
        // sm include directive?
        if (line.rfind("{{$INCLUDE ", 0) == 0) {
            // Get the three letter code.
            std::cout << evaluate(directory, line, n) << std::endl;
        } else 
            std::cout << line << std::endl; // Just dump it.
        ++n;
    }

    tpls.close();

    return success;
}