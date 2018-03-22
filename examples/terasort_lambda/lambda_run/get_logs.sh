rm -f hasher_logs sorter_logs

aws logs filter-log-events \
      --log-group-name /aws/lambda/cirrus_terasort_hasher \
      --output text > hasher_logs || rm -f hasher_logs
aws logs filter-log-events \
      --log-group-name /aws/lambda/cirrus_terasort_sorter \
      --output text > sorter_logs || rm -f sorter_logs

aws logs delete-log-group --log-group-name /aws/lambda/cirrus_terasort_hasher
aws logs delete-log-group --log-group-name /aws/lambda/cirrus_terasort_sorter
