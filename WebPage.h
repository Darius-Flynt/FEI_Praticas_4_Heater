#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

// Função que devolve o HTML
String htmlPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>HeatCooler ESP32 Webserver</title>
    <style>
      body {
        text-align: center;
        font-family: Arial, sans-serif;
        background-color: azure;
        color: rgb(63, 63, 63);
        margin: 0;
        padding: 0;
      }
      h1 {
        margin: 20px 0;
      }
      p {
        font-size: 1.2em;
        margin: 10px 0;
      }
      .status span {
        display: block;
        margin: 10px 0;
        font-size: 1.2em;
      }
      .card {
        max-width: 400px;
        margin: 20px auto;
        border-radius: 12px;
        overflow: hidden;
        box-shadow: 0 4px 10px rgba(0,0,0,0.1);
      }
      .card-header {
        background-color: rgb(17, 72, 91);
        color: azure;
        padding: 15px;
      }
      .card-body {
        background-color: rgb(161, 161, 161);
        padding: 20px;
      }
      form {
        display: flex;
        flex-direction: column;
        align-items: center;
      }
      label {
        font-weight: bold;
        margin-bottom: 5px;
      }
      input[type="number"] {
        width: 100%;
        padding: 10px;
        font-size: 1em;
        border-radius: 8px;
        border: 1px solid #ccc;
        margin-bottom: 15px;
      }
      input[type="submit"] {
        width: 100%;
        padding: 12px;
        font-size: 1.1em;
        background-color: rgb(17, 72, 91);
        color: azure;
        border: none;
        border-radius: 8px;
        cursor: pointer;
      }
      input[type="submit"]:hover {
        background-color: rgb(25, 100, 120);
      }
      .btn-group {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 10px;
        margin-top: 15px;
      }
      .btn-group a button {
        width: 100%;
        padding: 12px;
        font-size: 1.1em;
        border: none;
        border-radius: 8px;
        cursor: pointer;
        background-color: rgb(63, 63, 63);
        color: azure;
      }
      .btn-group a button:hover {
        background-color: rgb(90, 90, 90);
      }
    </style>
  </head>
  <body>
    <h1>Heat Cooler</h1>
    <p><b>Temperatura Ambiente:</b> 28,5°C</p>
    <p><b>Temperatura SetPoint:</b> 23,0°C</p>
    
    <div class="status">
      <span><b>Status Cooler FAN:</b> <span style="color: chartreuse;">LIGADO</span></span>
      <span><b>Status Heat Lamp:</b> <span style="color: rgb(244, 1, 1);">DESLIGADO</span></span>
    </div>

    <div class="card">
      <div class="card-header">
        <h2>Tipo de Controle</h2>
        <div class="btn-group">
          <a href='/ligaFan'><button>Manual</button></a>
          <a href='/desligaFan'><button>Automático</button></a>
        </div>
      </div>
      <div class="card-body">
        <form action="/setpoint" method="GET">
          <label for="sp">Setpoint:</label>
          <input type="number" id="sp" name="value" value="25">
          <input type="submit" value="Enviar">
        </form>
        <div class="btn-group">
          <a href='/ligaFan'><button>Liga FAN</button></a>
          <a href='/desligaFan'><button>Desliga FAN</button></a>
          <a href='/ligaHeat'><button>Liga HEAT</button></a>
          <a href='/desligaHeat'><button>Desliga HEAT</button></a>
        </div>
      </div>
    </div>
  </body>
</html>
  )rawliteral";

  return html;
}

#endif
