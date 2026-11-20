#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <vector>
#include <map>

using namespace std;

struct SVGIssue {
    int lineNumber;
    string issueType;
    string description;
    string lineContent;
};

class SVGScanner {
private:
    string filename;
    vector<string> lines;
    vector<SVGIssue> issues;
    
public:
    SVGScanner(string file) : filename(file) {}
    
    bool scan() {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << filename << endl;
            return false;
        }
        
        string line;
        int lineNumber = 0;
        bool xmlnsFound = false;
        string svgTagLine;
        
        while (getline(file, line)) {
            lineNumber++;
            lines.push_back(line);
            
            // Check for xmlns in svg tag
            if (line.find("<svg") != string::npos) {
                svgTagLine = line;
                if (line.find("xmlns=\"http://www.w3.org/2000/svg\"") != string::npos) {
                    xmlnsFound = true;
                }
            }
            
            // Check for fill="none"
            if (line.find("fill=\"none\"") != string::npos || line.find("fill='none'") != string::npos) {
                issues.push_back({lineNumber, "FILL_NONE", 
                    "fill=\"none\" found - polygon may not be visible", line});
            }
            
            // Check for fill="transparent"
            if (line.find("fill=\"transparent\"") != string::npos || line.find("fill='transparent'") != string::npos) {
                issues.push_back({lineNumber, "FILL_TRANSPARENT", 
                    "fill=\"transparent\" found - polygon may not be visible", line});
            }
            
            // Check for fill="rgba(0,0,0,0)"
            if (line.find("rgba(0,0,0,0)") != string::npos) {
                issues.push_back({lineNumber, "FILL_RGBA_ZERO", 
                    "fill=\"rgba(0,0,0,0)\" found - polygon is transparent", line});
            }
            
            // Check for clip-path
            if (line.find("clip-path=") != string::npos) {
                issues.push_back({lineNumber, "CLIP_PATH", 
                    "clip-path found - may hide some polygons", line});
            }
            
            // Check for polygons without fill attribute
            if (line.find("<polygon") != string::npos && line.find("points=") != string::npos) {
                if (line.find("fill=") == string::npos) {
                    issues.push_back({lineNumber, "MISSING_FILL", 
                        "polygon has no fill attribute - may not be visible", line});
                }
            }
            
            // Check for path elements without fill
            if (line.find("<path") != string::npos) {
                if (line.find("fill=") == string::npos) {
                    issues.push_back({lineNumber, "MISSING_FILL_PATH", 
                        "path has no fill attribute - may not be visible", line});
                }
            }
            
            // Check for overflow="hidden" that might cut off content
            if (line.find("overflow=\"hidden\"") != string::npos) {
                issues.push_back({lineNumber, "OVERFLOW_HIDDEN", 
                    "overflow=\"hidden\" found - may cut off polygons outside viewBox", line});
            }
        }
        
        file.close();
        
        // Check for missing xmlns
        if (!xmlnsFound && svgTagLine.find("<svg") != string::npos) {
            issues.push_back({1, "MISSING_XMLNS", 
                "SVG tag missing xmlns attribute", svgTagLine});
        }
        
        return true;
    }
    
    void printReport() {
        if (issues.empty()) {
            cout << "\n✓ No common SVG issues found in " << filename << endl;
            return;
        }
        
        cout << "\n=== SVG Scanner Report for: " << filename << " ===" << endl << endl;
        cout << "Found " << issues.size() << " potential issues:" << endl << endl;
        
        // Group by issue type
        map<string, vector<SVGIssue>> grouped;
        for (const auto& issue : issues) {
            grouped[issue.issueType].push_back(issue);
        }
        
        int issueNum = 1;
        for (const auto& pair : grouped) {
            cout << "--- " << pair.first << " (" << pair.second.size() << " occurrences) ---" << endl;
            
            for (const auto& issue : pair.second) {
                cout << issueNum++ << ". Line " << issue.lineNumber << ": " << issue.description << endl;
                cout << "   Content: " << issue.lineContent << endl << endl;
            }
            cout << endl;
        }
        
        // Summary with fixes
        cout << "=== RECOMMENDED FIXES ===" << endl << endl;
        for (const auto& pair : grouped) {
            cout << pair.first << ":" << endl;
            if (pair.first == "FILL_NONE" || pair.first == "FILL_TRANSPARENT" || pair.first == "FILL_RGBA_ZERO") {
                cout << "  -> Change fill to an explicit color: fill=\"#000000\"" << endl;
            } else if (pair.first == "MISSING_FILL" || pair.first == "MISSING_FILL_PATH") {
                cout << "  -> Add fill attribute: fill=\"#000000\"" << endl;
            } else if (pair.first == "CLIP_PATH") {
                cout << "  -> Check if clip-path is cutting off polygons, remove or adjust it" << endl;
            } else if (pair.first == "MISSING_XMLNS") {
                cout << "  -> Add xmlns to svg tag: <svg xmlns=\"http://www.w3.org/2000/svg\">" << endl;
            } else if (pair.first == "OVERFLOW_HIDDEN") {
                cout << "  -> Change to overflow=\"visible\" or remove the attribute" << endl;
            }
            cout << endl;
        }
    }
    
    int getIssueCount() {
        return issues.size();
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "SVG Scanner - Detects common SVG polygon visibility issues" << endl;
        cout << "Usage: " << argv[0] << " <svg_file>" << endl;
        cout << "Example: " << argv[0] << " mychart.svg" << endl;
        return 1;
    }
    
    string filename = argv[1];
    SVGScanner scanner(filename);
    
    cout << "Scanning " << filename << "..." << endl;
    
    if (!scanner.scan()) {
        return 1;
    }
    
    scanner.printReport();
    
    return 0;
}
