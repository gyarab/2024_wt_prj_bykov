FROM python:3.13-slim

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1 

RUN mkdir /npws
WORKDIR /npws

RUN pip install --upgrade pip
COPY requirements.txt /npws/
RUN pip install -r requirements.txt

COPY . /npws/

EXPOSE 8002

CMD ["python", "manage.py", "makemigrations", "main"]
CMD ["python", "manage.py", "migrate", "main"]

CMD ["python", "manage.py", "runserver", "0:0:0:0:8002"]