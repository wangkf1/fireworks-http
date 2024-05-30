Build: 
```
docker build . -t fireworks-http
```

Run:
```
docker run fireworks-http
```

Debug inside docker's bash environment:
```
docker run --rm -it --entrypoint bash fireworks-http 
```
