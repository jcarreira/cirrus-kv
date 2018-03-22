rm -f outputfile*.txt
rm -f bundle.zip

aws s3 rm s3://cirrus-terasort-bucket --recursive --exclude '*' --include '*FLAG*' > /dev/null
aws s3 rm s3://cirrus-terasort-bucket --recursive --exclude '*' --include 'SORTED*' > /dev/null
aws s3 rm s3://cirrus-terasort-bucket --recursive --exclude '*' --include 'TEST*' > /dev/null
