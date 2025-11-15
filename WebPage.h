#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

String htmlPage(Supervisorio &sup) {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>HeatCooler ESP32 Webserver</title>
<style>
  body {
    margin: 0;
    padding: 0;
    font-family: Arial, sans-serif;
    background-color: #f0f4f8;
    color: #333;
    display: flex;
    min-height: 100vh;
    overflow-x: hidden;
  }

  /* Container principal */
  .main-wrapper {
    display: flex;
    width: 100%;
  }

  /* Conteúdo principal (esquerda) */
  .main-content {
    flex: 1;
    padding: 20px;
    display: flex;
    flex-direction: column;
    align-items: center;
    overflow-y: auto;
  }

  .main-content h1 {
    color: #11485b;
    margin-bottom: 20px;
    text-align: center;
  }

  /* Status section */
  .status {
    background-color: #fff;
    padding: 15px;
    border-radius: 8px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    margin-bottom: 20px;
    width: 100%;
    max-width: 600px;
  }

  .status span {
    display: block;
    margin: 8px 0;
    font-size: 1.1em;
  }

  /* Card central */
  .control-card {
    width: 100%;
    max-width: 800px;
    border-radius: 12px;
    overflow: hidden;
    background-color: #fff;
    box-shadow: 0 6px 15px rgba(0,0,0,0.1);
  }
  
  .control-card .card-header {
    background-color: #11485b;
    color: #fff;
    padding: 15px;
    font-size: 1.2em;
    text-align: center;
  }
  
  .control-card .card-body {
    padding: 20px;
    display: flex;
    flex-wrap: wrap;
    gap: 20px;
  }
  
  .control-column {
    flex: 1;
    min-width: 200px;
  }
  
  .control-column h3 {
    text-align: center;
    color: #11485b;
    margin-bottom: 15px;
    padding-bottom: 8px;
    border-bottom: 1px solid #eee;
  }
  
  .btn-control {
    display: block;
    width: 100%;
    padding: 12px;
    margin-bottom: 10px;
    border: none;
    border-radius: 8px;
    background-color: #3f3f3f;
    color: #fff;
    cursor: pointer;
    font-size: 1em;
    transition: background-color 0.3s;
    text-align: center;
  }
  
  .btn-control:hover {
    background-color: #5a5a5a;
  }
  
  input[type="number"], input[type="text"] {
    width: 100%;
    padding: 10px;
    margin-bottom: 15px;
    border-radius: 8px;
    border: 1px solid #ccc;
    font-size: 1em;
  }
  
  input[type="submit"] {
    width: 100%;
    padding: 12px;
    border-radius: 8px;
    border: none;
    background-color: #11485b;
    color: #fff;
    cursor: pointer;
    font-size: 1em;
    transition: background-color 0.3s;
  }
  
  input[type="submit"]:hover {
    background-color: #0d3745;
  }

  /* Sidebar usuários (direita) */
  .sidebar {
    width: 300px;
    background-color: #fff;
    border-left: 1px solid #ccc;
    padding: 20px;
    overflow-y: auto;
    height: 100vh;
    position: sticky;
    top: 0;
  }
  
  .sidebar h2 {
    text-align: center;
    margin-top: 0;
    color: #11485b;
    padding-bottom: 10px;
    border-bottom: 1px solid #ccc;
  }
  
  .user-card {
    padding: 12px;
    margin-bottom: 12px;
    border-bottom: 1px solid #eee;
    background-color: #f9f9f9;
    border-radius: 6px;
  }
  
  .user-card label {
    font-weight: bold;
    display: block;
    margin-bottom: 5px;
    color: #11485b;
  }
  
  .user-card input {
    margin-bottom: 8px;
  }

  /* Responsividade */
  @media (max-width: 900px) {
    .main-wrapper {
      flex-direction: column;
    }
    
    .sidebar {
      width: 100%;
      height: auto;
      border-left: none;
      border-top: 1px solid #ccc;
    }
    
    .control-card .card-body {
      flex-direction: column;
    }
  }
</style>
</head>
<body>

<div class="main-wrapper">
  <div class="main-content">
    <h1>Heat Cooler</h1>

    <div class="status">
)rawliteral";

  html += "<span><b>Temperatura Ambiente:</b> " + String(sup.temp, 1) + " °C</span>";
  html += "<span><b>Setpoint Geral:</b> " + String(sup.setpoint, 1) + " °C</span>";
  html += "<span><b>Status FAN:</b> <span style='color:" + String(sup.fanStatus ? "green'>LIGADO" : "red'>DESLIGADO") + "</span></span>";
  html += "<span><b>Status Lamp:</b> <span style='color:" + String(sup.lampStatus ? "green'>LIGADA" : "red'>DESLIGADA") + "</span></span>";
  html += "</div>";

  // Card central de controle com 3 colunas
  html += R"rawliteral(
    <div class="control-card">
      <div class="card-header">Tipo de Controle</div>
      <div class="card-body">
        <!-- Coluna da Esquerda -->
        <div class="control-column">
          <h3>Controle Manual</h3>
          <a href='/manual'><button class="btn-control">Manual</button></a>
          <a href='/ligaFan'><button class="btn-control">Liga FAN</button></a>
          <a href='/desligaFan'><button class="btn-control">Desliga FAN</button></a>
          <a href='/ligaHeat'><button class="btn-control">Liga LAMP</button></a>
          <a href='/desligaHeat'><button class="btn-control">Desliga LAMP</button></a>
        </div>
        
        <!-- Coluna do Meio -->
        <div class="control-column">
          <h3>Controle Automático</h3>
          <a href='/automatico'><button class="btn-control">Automático</button></a>
          <form action="/setpoint" method="GET">
            <label for="sp">Temperatura:</label>
            <input type="number" id="sp" name="value" step="0.1" value=)rawliteral";

  html += String(sup.setpoint, 1);

  html += R"rawliteral(>
            <input type="submit" value="Enviar">
          </form>
        </div>
        
        <!-- Coluna da Direita -->
        <div class="control-column">
          <h3>Controle por Acesso</h3>
          <a href='/acesso'><button class="btn-control">Controle de Acesso</button></a>
        </div>
      </div>
    </div>
  </div>
)rawliteral";

  // Sidebar lateral de usuários
  html += R"rawliteral(
  <div class="sidebar">
    <h2>Usuários Cadastrados</h2>
    <form action='/updateUser' method='GET'>
)rawliteral";

  for (int i = 0; i < sup.validUsers; i++) {
    html += "<div class='user-card'>";
    html += "<div style='display:flex; align-items:center; justify-content:space-between;'>";
    html += "<label for='name" + String(i) + "'>Nome:</label>";

    // Bolinha de status
    if (sup.users[i].presente) {
      html += "<span style='display:inline-block; width:12px; height:12px; border-radius:50%; background-color:green; margin-left:8px;'></span>";
    } else {
      html += "<span style='display:inline-block; width:12px; height:12px; border-radius:50%; background-color:red; margin-left:8px;'></span>";
    }
    html += "</div>";

    html += "<input type='text' id='name" + String(i) + "' name='name" + String(i) + "' value='" + sup.users[i].Nome + "'>";
    html += "<label for='spUser" + String(i) + "'>Setpoint:</label>";
    html += "<input type='number' id='spUser" + String(i) + "' name='spUser" + String(i) + "' step='0.1' value='" + String(sup.users[i].Temperatura, 1) + "'>";
    html += "<input type='hidden' name='userIndex" + String(i) + "' value='" + String(i) + "'>";
    html += "</div>";
  }

  html += "<input type='submit' value='Atualizar Usuários'>";
  html += "</form></div></div>";

  // Reload Automatico
  html += R"rawliteral(
<script>
setInterval(() => {
  fetch('/checkUpdate')
    .then(res => res.text())
    .then(t => {
      if (t === 'reload') location.reload();
    })
    .catch(err => {
      // ignora erros de conexão breves
      // console.log('checkUpdate err', err);
    });
}, 1000); // verifica a cada 1 segundo
</script>
</body></html>
)rawliteral";

  return html;
}

#endif
