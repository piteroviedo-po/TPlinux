#usar base de datos
USE mysql;

#crear tabla
CREATE TABLE IF NOT EXISTS usuarios (
  id INT AUTO_INCREMENT PRIMARY KEY,
   nombre VARCHAR (100) NOT NULL,
   email VARCHAR (100) NOT NULL
);

#insertar datos 

INSERT INTO usuarios (nombre, email) VALUES
 ('juan', 'juan@palermo.com'),
 ('roberto' ,'roberto@palermo.com');
