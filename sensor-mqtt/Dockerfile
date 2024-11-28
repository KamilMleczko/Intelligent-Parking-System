FROM gradle:8.11-jdk23
COPY . .
RUN ./gradlew --no-daemon installDist -x test
ENTRYPOINT ["./build/install/parking/bin/parking"]