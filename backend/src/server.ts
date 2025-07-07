import * as fs from 'fs';
import cors from 'cors'
import * as path from 'path';
import * as coap from 'coap';
import express from 'express';
import axios from 'axios'
import FormData from 'form-data'


// ————— Configuração do Telegram —————
const BOT_TOKEN = '7592756051:AAEGMzHJQTSOSrUGyi9_tejnsl9a6NQzSW8'
const CHAT_ID  = '711984191'  // ex: '987654321'
const TELEGRAM_API = `https://api.telegram.org/bot${BOT_TOKEN}`

async function sendTelegram(text: string) {
  try {
    await axios.post(`${TELEGRAM_API}/sendMessage`, {
      chat_id: CHAT_ID,
      text,
      parse_mode: 'Markdown'
    })
    console.log('[Telegram] mensagem enviada:', text)
  } catch (err) {
    console.error('[Telegram] falha ao enviar mensagem:', err)
  }
}


async function sendTelegramPhotoUpload() {
  const filePath = path.join(__dirname, 'public', 'images', 'latest.jpg')
  const form = new FormData()
  form.append('chat_id', CHAT_ID.toString())
  form.append('caption', '📷 Foto mais recente do sensor')
  form.append('photo', fs.createReadStream(filePath))

  try {
    await axios.post(
      `${TELEGRAM_API}/sendPhoto`,
      form,
      { headers: form.getHeaders() }
    )
    console.log('[Telegram] foto enviada via upload')
  } catch (err) {
    console.error('[Telegram] erro ao enviar foto:', err)
  }
}


const IMAGE_DIR = path.join(__dirname, 'public', 'images');
fs.mkdirSync(IMAGE_DIR, { recursive: true });

let receivedChunks: Buffer[] = [];

// Variáveis globais para guardar os valores
let lastDistance: number | null = null
let lastHumidity: number | null = null


const coapServer = coap.createServer();

coapServer.on('request', (req, res) => {
  console.log(
    `📬 CoAP request de ${req.rsinfo.address}:${req.rsinfo.port} —`,
    req.method,
    req.url
  );

  if (req.method === 'POST' && req.url === '/image') {
    if (!req.payload || req.payload.length < 1) {
      console.log("Payload inválido.");
      res.end("Bad Request");
      return;
    }

    const chunkId = req.payload[0];

    if (chunkId === 255) {
      // 🔥 EOF chegou
      const finalImage = Buffer.concat(receivedChunks.filter(Boolean));
      const filePath = path.join(IMAGE_DIR, 'latest.jpg');
      fs.writeFileSync(filePath, finalImage);
      console.log(`✅ Imagem reconstruída em ${filePath} (${finalImage.length} bytes)`);
      receivedChunks = [];
      res.end("Imagem salva!");
      return;
    }

    const chunkData = req.payload.slice(1);
    receivedChunks[chunkId] = chunkData;
    console.log(`📥 Chunk ${chunkId} recebido, size=${chunkData.length}`);

    res.end('OK');
  } else {
    res.code = '4.04';
    res.end('Not found');
  }


  if (req.method === 'POST' && req.url === '/distance') {
    if (!req.payload || req.payload.length < 1) {
      console.log("Payload inválido.");
      res.end("Bad Request");
      return;
    }

    const payload = req.payload as string;        // e.g. "28.58"
    const distance = parseFloat(payload)
    
    console.log(distance)
    lastDistance = distance
    
    if (distance < 16) {
      sendTelegram(`⚠️ *Deslizamento!* Distância crítica de *${distance.toFixed(2)} cm* detectada.`)
      sendTelegramPhotoUpload() 
    }
    // console.log(req)
  }

  if (req.method === 'POST' && req.url === '/humidity') {
    if (!req.payload || req.payload.length < 1) {
      console.log("Payload inválido.");
      res.end("Bad Request");
      return;
    }

    const payload = req.payload as string;        // e.g. "28.58"
    const humidity = parseFloat(payload)
    
    console.log(`${humidity} %`)
    lastHumidity = humidity
    // console.log(req)
  }


});

coapServer.listen(() => console.log('CoAP server ouvindo na porta UDP 5683'));

const app = express();
app.use(cors())

app.use('/images', express.static(path.join(__dirname, 'public', 'images')));


app.get('/distance', (req, res) => {
  if (lastDistance === null) {
    return res.status(404).json({ error: 'sem leitura ainda' })
  }
  res.json({ distance: lastDistance })
})

// GET /humidity -> retorna última umidade
app.get('/humidity', (req, res) => {
  if (lastHumidity === null) {
    return res.status(404).json({ error: 'sem leitura ainda' })
  }
  res.json({ humidity: lastHumidity })
})


const HTTP_PORT = 3000;
app.listen(HTTP_PORT, () =>
  console.log(`🌐 HTTP server ouvindo em http://localhost:${HTTP_PORT}`)
);

