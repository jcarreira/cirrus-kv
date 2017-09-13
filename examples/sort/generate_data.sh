# to generate 1TB of data

# use code from https://github.com/ehiggs/spark-terasort

PATH_TO_SPARK_TERASORT=/data/joao/spark-terasort
OUTPUT_PATH=/data/joao/cirrus/examples/sort/terasort_in

/usr/local/spark/bin/spark-submit --class com.github.ehiggs.spark.terasort.TeraGen \
     ${PATH_TO_SPARK_TERASORT}/target/spark-terasort-1.0-jar-with-dependencies.jar \
     1g ${OUTPUT_PATH}

