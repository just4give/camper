export AWS_PROFILE=bikemate
export TG_TOKEN=your_tg_token
export TG_CHAT_ID=your_tg_chat_id
cd src/
rm index.zip
pip install --target ./package -r requirements.txt
cd package
zip -r ../index.zip .
cd ..
zip -g index.zip lambda_function.py

aws lambda update-function-code --function-name CamperHeliumChannel --zip-file fileb://index.zip
aws lambda update-function-configuration --function-name CamperHeliumChannel --timeout 30 --environment "Variables={TG_TOKEN=$TG_TOKEN,TG_CHAT_ID=$TG_CHAT_ID}"