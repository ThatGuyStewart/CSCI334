# CSCI334

All files and folders except /src are required to run the .exe

SQLpostgre settings:
  host: localhost
  port: 5432
  database: PMS
  username: postgres
  password: data

So long as SQLpostgre is running with these values, the program will automatically connect to the database, create the schema, and seed test data.
Once the server is running, connect to https://127.0.0.1:8080/test in your browser to view the test page.
You will get a warning that the author of the site is unauthenticated. That is because it's using a self-signed certificate for SSL.
