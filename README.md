# json-thresher
C++ json binder

## To build and test

Note: currently working on a new package manager to use on this package.  Other managers should still 
work(as far as they do) as https://github.com/DataDriven-CAM/cnpm.git is based on the package.json npm format.

### cnpm building

```
cnpm  install

cnpm lib

#building and running unit tests
cnpm test

```
## Examples

### Getting the value of a key

```
        std::ifstream is("../package.json");
        std::string jsonContent((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        jsonBinder(jsonContent);
        sylvanmats::io::json::Path jpName;
        jpName["name"];
        std::string_view currentPackageName;
        //the get operator
        jsonBinder(jpName, [&currentPackageName](std::any& v){
            currentPackageName=std::any_cast<std::string_view>(v);
        });
```
finds the package name; currentPackageNameshould equal "json-thresher".

### Add an object and properties

```
    sylvanmats::io::json::Binder jsonBinder;
    sylvanmats::io::json::Path jp;
    jsonBinder(jp, "8DR", sylvanmats::io::json::object());
    jp["8DR"];
    jsonBinder(jp, "start", 100);
    jsonBinder(jp, "end", 200);
```
generates a json:
```
{
    "8DR": {
        "start": 100,
        "end": 200
    }
}

```

### Finding an elements properties
```
    sylvanmats::io::json::Path jpSymbol;
    jpSymbol["elements"]["*"]["symbol"]=="C";

```
matches any object in the elements array where symbol is C; and from periodic table [Periodic-Table-JSON](https://github.com/Bowserinator/Periodic-Table-JSON.git) 
and traverses all value pairs for the element by ```jsonBinder(jpSymbol, [&](std::string_view& key, std::any& v){});```.  

## Contact

My twitch stream is https://www.twitch.tv/sylvanmats
(with some hardware hope to get active)