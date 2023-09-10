<template>
  <div class="container">
    <div class="row">
      <img src="/assets/logo.svg" class="logo" alt="Molybden logo"/>
    </div>
    <h1>Speakly</h1>
    <p>Please enter your name and click the button.</p>
    <div class="row">
      <div>
        <input id="greet-input" placeholder="Your name"/>
        <button id="greet-btn" type="button">Greet</button>
      </div>
    </div>
    <p id="greet-msg"></p>
  </div>
</template>

<script>
function sayHello() {
  const name = document.querySelector("#greet-input").value;
  document.querySelector("#greet-msg").textContent = greet(name);
}

window.addEventListener("DOMContentLoaded", () => {
  const greetInput = document.querySelector("#greet-input");
  const btn = document.querySelector("#greet-btn");

  greetInput.addEventListener('keypress', (event) => {
    if (event.key === 'Enter') {
      btn.click();
    }
  });

  btn.addEventListener("click", () => sayHello());
});

export default {
  name: "App"
}
</script>
