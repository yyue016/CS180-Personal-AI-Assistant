document.addEventListener("DOMContentLoaded", function () {
    const fields = [
        "displayName",
        "language",
        "responseTone",
        "answerLength",
        "examplePreferences",
        "aiModel"
    ];

    const saveButton = document.getElementById("saveSettings");

    if (!saveButton) return;

    const form = document.getElementById("settingsForm");

    if (form) {
        form.addEventListener("submit", function (event) {
            event.preventDefault();
        });
    }

    fields.forEach(function (id) {
        const element = document.getElementById(id);
        const savedValue = localStorage.getItem(id);

        if (element && savedValue !== null) {
            element.value = savedValue;
        }
    });

    saveButton.addEventListener("click", function () {
        fields.forEach(function (id) {
            const element = document.getElementById(id);

            if (element) {
                localStorage.setItem(id, element.value);
            }
        });

        alert("Settings saved successfully!");
    });
});