FROM debian:bullseye
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    apache2 \
    libapache2-mod-fcgid \
    g++ \
    default-mysql-client \
    libmariadb-dev \
    libmariadb-dev-compat \
    curl \
 && rm -rf /var/lib/apt/lists/*
RUN a2enmod cgi
WORKDIR /app/cgi-bin
COPY cgi-bin/ /app/cgi-bin/
RUN g++ -Wall register_login.cpp -o register_login.cgi -I/usr/include/mariadb -lmariadb && \
    g++ -Wall lostfound_submit.cpp -o lostfound_submit.cgi -I/usr/include/mariadb -lmariadb && \
    chmod +x *.cgi && \
    echo "✅ Compiled CGI files in /app/cgi-bin:" && ls -lh
WORKDIR /var/www/html
COPY html/ /var/www/html/
RUN chown -R www-data:www-data /var/www/html /usr/lib/cgi-bin
EXPOSE 80
COPY wait-for-it.sh /wait-for-it.sh
RUN chmod +x /wait-for-it.sh
CMD ["sh", "-c", "cp /app/cgi-bin/*.cgi /usr/lib/cgi-bin/ && /wait-for-it.sh db:3306 -- apachectl -D FOREGROUND"]