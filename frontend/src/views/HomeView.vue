<template>
  <div
    style="display: flex;
           flex-direction: column;
           gap: 20px;
           padding: 50px;
           border-radius: 10px;
           box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1),
                       0 8px 20px rgba(0, 0, 0, 0.1);"
  >
    <div style="font-size: 24px; font-weight: bold;">Sensoriamento</div>

    <div style="display: flex; flex-direction: column; gap: 10px; justify-content: center; align-items: center;">
      <!-- Distância -->
      <div style="display: flex; flex-direction: column; align-items: start;">
        <span style="font-size: 20px; font-weight: bold;">
          Distância: {{ distance }} cm
        </span>
        <div
          style="font-size: 20px;
                 color: white;
                 font-weight: bold;
                 height: 50px;
                 width: 240px;
                 border: 3px solid black;
                 border-radius: 10px;
                 overflow: hidden;
                 display: flex;
                 justify-content: center;
                 align-items: center;"
          :style="{
            'background-color':
              distance < 16
                ? 'red'
                : distance < 20
                ? 'orange'
                : 'darkgreen'
          }"
        >
          {{
            distance < 16
              ? 'Perigo!'
              : distance < 20
              ? 'Alerta'
              : 'Seguro'
          }}
        </div>
      </div>

      <!-- Umidade -->
      <div style="display: flex; flex-direction: column; align-items: start;">
        <span style="font-size: 20px; font-weight: bold;">
          Humidade: {{ humidity }} %
        </span>
        <div
          style="height: 50px;
                 width: 240px;
                 border: 3px solid black;
                 border-radius: 10px;
                 overflow: hidden;
                 display: flex;
                 align-items: flex-end;"
        >
          <div
            style="height: 100%;
                   background-color: aqua;
                   transition: width 0.5s ease;"
            :style="{ width: humidity + '%' }"
          ></div>
        </div>
      </div>
    </div>

    <!-- Imagem do sensor -->
    <div 
      v-if="distance < 16"
    style="display: flex; flex-direction: column; align-items: center; gap: 10px;">
      <span style="font-size: 20px; font-weight: bold;">Última Imagem</span>
      <img
        :src="imageUrl"
        alt="Imagem do sensor"
        style="width: 512px;
               height: auto;
               border: 3px solid #333;
               border-radius: 10px;
               box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1),
                           0 8px 20px rgba(0, 0, 0, 0.1);"
      />
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, onMounted, onBeforeUnmount } from 'vue'
import axios from 'axios'

const api = axios.create({
  baseURL: 'http://localhost:3000',
  timeout: 5000,
})

const distance = ref<number | null>(null)
const humidity = ref<number | null>(null)
const imageUrl = ref<string>('')

let sensorTimer: number
let imageTimer: number

async function fetchReadings() {
  try {
    const [dRes, hRes] = await Promise.all([
      api.get<{ distance: number }>('/distance'),
      api.get<{ humidity: number }>('/humidity'),
    ])
    distance.value = dRes.data.distance
    humidity.value = hRes.data.humidity
  } catch (err) {
    console.error('Erro ao buscar leituras:', err)
  }
}

function refreshImage() {
  // Cache-busting com timestamp
  imageUrl.value = `http://localhost:3000/images/latest.jpg?t=${Date.now()}`
}

onMounted(() => {
  // Sensoriamento a cada 2 s
  fetchReadings()
  sensorTimer = window.setInterval(fetchReadings, 2000)

  // Atualiza imagem a cada 2 s
  refreshImage()
  imageTimer = window.setInterval(refreshImage, 2000)
})

onBeforeUnmount(() => {
  clearInterval(sensorTimer)
  clearInterval(imageTimer)
})
</script>

<style scoped>
* {
  padding: 0;
  margin: 0;
}
</style>
