/* --- 전역 변수 및 요소 가져오기 --- */
const galleryContainer = document.getElementById('galleryContainer');
const backdrop = document.getElementById('backdrop');
const colsInput = document.getElementById('colsInput');
const rowsInput = document.getElementById('rowsInput');
const imageUrlsInput = document.getElementById('imageUrls');
const updateGridBtn = document.getElementById('updateGridBtn');
// const alertSound = new Audio("data:audio/wav;base64,UklGRl9vT19XQVZFZm10IBAAAAABAAEAQB8AAEAfAAABAAgAZGF0YU");
const alarmSound = new Audio('./resource/alarm.mp3');
alarmSound.loop = true;

const AlarmType = {
    0: "standing",
    1: "lying down on bed",
    2: "sitting on bed",
    3: "fallen down",
    4: "wheel chair",
    5: "unknown status",
    6: "sitting on chair",
    7: "sitting on the floor",
    8: "food tray",
    9: "perch on bed",
    10: "staff",
}

async function startAlarm() {
    try {
        await alarmSound.play();
        console.log("알람 재생 시작");
    } catch (err) {
        console.error("재생 실패 (브라우저 정책 등):", err);
    }
}

function stopAlarm() {
    alarmSound.pause();
    alarmSound.currentTime = 0; // 재생 위치를 처음으로 되돌림
    console.log("알람 정지");
}


function toggleFullScreen() {
    if (!document.fullscreenElement) {
        document.documentElement.requestFullscreen().catch(err => {
            console.log(`Error: ${err.message}`);
        });
    }
}

// --- 추가된 UI 요소 ---
const openSettingsBtn = document.getElementById('openSettingsBtn');
const closeSettingsBtn = document.getElementById('closeSettingsBtn');
const settingsModal = document.getElementById('settingsModal');

// --- 갤러리 기능 함수화 ---

/**
 * 갤러리 아이템에 클릭 시 확대/축소되는 이벤트 리스너를 설정하는 함수
 * 수정됨: cloneNode 삭제 (setInterval 참조 유지)
 */
function setupGalleryEventListeners() {
    // 이미 updateGrid에서 기존 DOM을 날리고 새로 만들었으므로,
    // 여기서 select되는 item들은 모두 리스너가 없는 새 요소들입니다.
    // 따라서 복제(cloning) 과정 없이 바로 addEventListener를 해도 안전합니다.
    const galleryItems = document.querySelectorAll('.gallery-item');

    galleryItems.forEach(item => {
        // 이미 클릭 이벤트가 있을 수 있으니 방어적으로 제거하거나, 
        // 현재 로직상 updateGrid가 매번 새로 만들므로 바로 추가해도 됩니다.

        item.addEventListener('click', () => {
            if (item.classList.contains('empty')) return;

            const currentFocused = document.querySelector('.gallery-item.focused');
            // 자기 자신이 이미 포커스 상태라면 해제, 아니면 포커스
            if (currentFocused && currentFocused !== item) {
                currentFocused.classList.remove('focused');
            }
            item.classList.toggle('focused');

            // 포커스 된 요소가 하나라도 있으면 백드롭 표시
            const isAnyFocused = document.querySelector('.gallery-item.focused');
            backdrop.style.display = isAnyFocused ? 'block' : 'none';
        });
    });
}

/* --- 전역 변수 수정 --- */
// [수정] 단순 텍스트 저장(Map)에서 '상태 객체' 저장으로 변경
// Key: txtUrl, Value: { lastText: "...", emptyCount: 0 }
const channelState = new Map();


function renderBox(canvas, text) {
    const ctx = canvas.getContext('2d');
    if (!text) return;

    ctx.clearRect(0, 0, canvas.width, canvas.height);
    const lines = text.split('\n');

    // 이번 프레임에서 알람이 발생했는지 체크하는 플래그
    let frameHasAlarm = false;

    lines.forEach(line => {
        const parts = line.trim().split(' ');
        if (parts.length >= 6) {
            const isAlarm = parseInt(parts[0]);
            const classId = parseInt(parts[1]);
            const x_center = parseFloat(parts[2]);
            const y_center = parseFloat(parts[3]);
            const w_norm = parseFloat(parts[4]);
            const h_norm = parseFloat(parts[5]);

            // 알람 객체가 하나라도 발견되면 플래그를 True로 설정
            if (isAlarm > 0) {
                frameHasAlarm = true;
            }

            const boxW = w_norm * canvas.width;
            const boxH = h_norm * canvas.height;
            const boxX = (x_center * canvas.width) - (boxW / 2);
            const boxY = (y_center * canvas.height) - (boxH / 2);

            ctx.beginPath();
            ctx.lineWidth = 2;

            // [수정 1] 경고 여부와 상관없이 박스는 항상 녹색
            ctx.strokeStyle = '#00FF00';
            ctx.fillStyle = '#00FF00';

            ctx.rect(boxX, boxY, boxW, boxH);
            ctx.stroke();

            ctx.save();
            ctx.font = 'bold 16px Arial';
            ctx.fillText(AlarmType[classId] || classId, boxX, boxY - 5);
            ctx.restore();
        }
    });

    // [수정 2] 알람 발생 시 부모 컨테이너(gallery-item)의 테두리 스타일 변경
    // canvas의 부모 요소(.gallery-item)를 찾습니다.
    const parentItem = canvas.parentElement;
    if (parentItem) {
        if (frameHasAlarm) {
            parentItem.classList.add('alarm-active'); // 깜빡이는 테두리 추가
            startAlarm();
        } else {
            parentItem.classList.remove('alarm-active'); // 테두리 제거
            stopAlarm();
        }
    }
}

async function sendRobotSignalStatus(channelId, isMuted) {
    try {
        // 실제 운영 환경의 서버 주소로 변경 필요 (예: http://localhost:8080/api/robot-control)
        const response = await fetch('http://localhost:8081/api/robot-control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                channel: channelId,
                muteSignal: isMuted
            })
        });
        console.log(`채널 ${channelId} 신호 차단 상태: ${isMuted}`);
    } catch (err) {
        console.error("서버 통신 실패:", err);
    }
}

/* --- 메인 그리기 함수 (로직 수정됨) --- */
function drawBBox(canvas, txtUrl) {
    const ctx = canvas.getContext('2d');

    // 1. 캔버스 크기 싱크 및 복구 로직
    const rect = canvas.parentElement.getBoundingClientRect();
    const isResized = (canvas.width !== rect.width || canvas.height !== rect.height);

    // 상태값 초기화 (처음 실행 시)
    if (!channelState.has(txtUrl)) {
        channelState.set(txtUrl, { lastText: '', emptyCount: 0 });
    }
    const state = channelState.get(txtUrl);

    if (isResized) {
        canvas.width = rect.width;
        canvas.height = rect.height;
        // 크기가 바뀌어서 지워졌으면, 기억해둔 내용으로 즉시 복구
        if (state.lastText) {
            renderBox(canvas, state.lastText);
        }
    }

    // 2. 데이터 가져오기
    fetch(txtUrl, { cache: 'no-store' })
        .then(res => {
            if (!res.ok) throw new Error('No Data');
            return res.text();
        })
        .then(text => {
            const isEmpty = (!text || text.trim().length === 0);

            // [⭐⭐ 핵심 수정: 빈 데이터 처리 로직 ⭐⭐]
            if (isEmpty) {
                state.emptyCount++; // 빈 횟수 카운트 증가

                // "2번 이상 연속"으로 비어있다면 -> 진짜 객체 없음으로 판단하고 지움
                // (약 0.5초~1초 정도의 딜레이가 생기지만, 깜빡임은 완벽 차단됨)
                if (state.emptyCount >= 2) {
                    ctx.clearRect(0, 0, canvas.width, canvas.height);
                    state.lastText = ''; // 상태 초기화

                    if (canvas.parentElement) {
                        canvas.parentElement.classList.remove('alarm-active');
                    }

                    // 카운트가 무한히 늘어나지 않게 고정
                    if (state.emptyCount > 10) state.emptyCount = 2;
                }

                // 1번만 비어있는 경우(글리치 가능성)는 무시하고 함수 종료 (이전 박스 유지)
                return;
            }

            // --- 데이터가 있는 경우 ---

            state.emptyCount = 0; // 빈 카운트 리셋 (연속성 깨짐)

            // 내용이 이전과 같으면 그리지 않음 (CPU 절약)
            if (state.lastText === text) {
                return;
            }

            // 데이터 업데이트 및 그리기
            state.lastText = text;
            renderBox(canvas, text);
        })
        .catch(err => {
            // 네트워크 에러 시에는 기존 그림 유지 (깜빡임 방지)
        });
}

/**
 * 입력된 설정에 따라 그리드를 새로 생성하는 함수
 */
function updateGrid() {
    const cols = parseInt(colsInput.value, 10);
    const rows = parseInt(rowsInput.value, 10);
    const imageUrls = imageUrlsInput.value.split('\n').filter(url => url.trim() !== '');
    const totalCells = cols * rows;

    galleryContainer.style.gridTemplateColumns = `repeat(${cols}, 1fr)`;
    galleryContainer.innerHTML = '';

    for (let i = 0; i < totalCells; i++) {
        // 1. 먼저 item 요소를 생성합니다. (순서 중요!)
        const item = document.createElement('div');
        item.classList.add('gallery-item');

        // 2. 체크박스 오버레이 생성
        const overlay = document.createElement('div');
        overlay.className = 'robot-control-overlay';

        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.id = `mute_robot_${i}`;
        // 기본값은 false (신호 보냄)

        const label = document.createElement('label');
        label.htmlFor = `mute_robot_${i}`;
        label.innerText = '로봇 신호 보내지 않기';

        // 클릭 이벤트 전파 방지 (확대/축소 방해 금지)
        overlay.addEventListener('click', (e) => e.stopPropagation());

        checkbox.addEventListener('change', (e) => {
            const isMuted = e.target.checked;
            sendRobotSignalStatus(i + 1, isMuted);
        });

        overlay.appendChild(checkbox);
        overlay.appendChild(label);

        // 3. item에 overlay를 추가합니다.
        item.appendChild(overlay);

        // --- 이하 기존 영상 처리 로직 ---
        let rawInput = imageUrls[i] ? imageUrls[i].trim() : '';
        let [videoUrl, txtUrl] = rawInput.split(',').map(s => s.trim());

        if (videoUrl) {
            if (videoUrl.startsWith('rtsp://')) {
                item.classList.add('stream-mode');
                const iframe = document.createElement('iframe');
                iframe.src = `/stream/stream.html?src=${encodeURIComponent(videoUrl)}&mode=webrtc`;
                iframe.classList.add('stream-iframe');

                // ... (중략: iframe 스타일 및 onload 로직) ...

                item.appendChild(iframe);

                if (txtUrl) {
                    const canvas = document.createElement('canvas');
                    // ... (중략: canvas 스타일 설정) ...
                    item.appendChild(canvas);

                    const drawInterval = setInterval(() => {
                        if (!document.body.contains(canvas)) {
                            clearInterval(drawInterval);
                            return;
                        }
                        drawBBox(canvas, txtUrl);
                    }, 500);
                }
            }
            // ... (나머지 m3u8, img 처리 로직) ...
        } else {
            item.classList.add('empty');
        }

        // 4. 완성된 item을 컨테이너에 추가
        galleryContainer.appendChild(item);
    }
    setupGalleryEventListeners();
}

// --- 이벤트 리스너 설정 ---

// 배경 클릭 시 열려있는 창 닫기
backdrop.addEventListener('click', () => {
    const focusedItem = document.querySelector('.gallery-item.focused');
    if (focusedItem) {
        focusedItem.classList.remove('focused');
        backdrop.style.display = 'none';
    }

    if (settingsModal.style.display === 'block') {
        settingsModal.style.display = 'none';
        backdrop.style.display = 'none';
    }
});

// "그리드 업데이트" 버튼 클릭 이벤트
updateGridBtn.addEventListener('click', () => {
    updateGrid();
    settingsModal.style.display = 'none';
    backdrop.style.display = 'none';
});

// '설정' 버튼 클릭 시 모달 열기
openSettingsBtn.addEventListener('click', () => {
    settingsModal.style.display = 'block';
    backdrop.style.display = 'block';
});

// 모달의 '닫기' 버튼 클릭 시 모달 닫기
closeSettingsBtn.addEventListener('click', () => {
    settingsModal.style.display = 'none';
    backdrop.style.display = 'none';
});

// --- 페이지 로드 시 초기 그리드 생성 ---
document.addEventListener('DOMContentLoaded', updateGrid);


// --- 기존 로그 및 알림 관련 코드 ---
let counter = 0;
// const counterElement = document.getElementById('counter');
// const textBox = document.getElementById('textBox');
let last_alarm = "";

function updateCloseAllButtonVisibility() {
    const container = document.getElementById('notification-container');
    const notifications = container.querySelectorAll('.toast-notification');
    let closeAllBtn = document.getElementById('close-all-btn');

    if (notifications.length >= 2) {
        if (!closeAllBtn) {
            closeAllBtn = document.createElement('button');
            closeAllBtn.id = 'close-all-btn';
            closeAllBtn.innerText = '알림 모두 닫기';

            closeAllBtn.onclick = () => {
                container.querySelectorAll('.toast-notification').forEach(n => {
                    n.classList.remove('show');
                    n.addEventListener('transitionend', () => n.remove());
                });
                closeAllBtn.classList.remove('show');
                closeAllBtn.addEventListener('transitionend', () => closeAllBtn.remove());
            };
        }
        container.appendChild(closeAllBtn);
        setTimeout(() => closeAllBtn.classList.add('show'), 10);
    } else if (closeAllBtn) {
        closeAllBtn.classList.remove('show');
        closeAllBtn.addEventListener('transitionend', () => closeAllBtn.remove());
    }
}

function createNotification(message) {
    const container = document.getElementById('notification-container');
    const notification = document.createElement('div');
    notification.className = 'toast-notification';

    // --- ✨ 수정된 부분: 정규식 및 헤더 생성 로직 변경 ---

    // 정규식을 사용해 '시간', 'Thread 번호', '본문'을 각각 별도의 그룹으로 추출합니다.
    // 예: "[2025-10-01 17:02:15 | Thread 1 | ALARM] | [ 1번 채널 ]..."
    const regex = /^\[(.*?)\s*\|\s*Thread\s*(\d+).*?\]\s*\|\s*(.*)$/;
    const match = message.match(regex);

    let headerText = '';
    let bodyText = '';

    if (match && match.length === 4) {
        // 정규식 매칭 성공
        const timestamp = match[1].trim(); // 그룹 1: 시간 (예: "2025-10-01 17:02:15")
        const channelNum = match[2].trim();  // 그룹 2: Thread 번호 (예: "1")

        // 추출한 정보로 원하는 헤더 형식을 새로 만듭니다.
        headerText = `${timestamp} | Channel ${channelNum}`;

        bodyText = match[3].trim();      // 그룹 3: "|" 뒷부분 전체
    } else {
        // 정규식 매칭 실패 시: 전체 메시지를 본문에 표시 (호환성 유지)
        bodyText = message;
    }

    // 최종적으로 완성된 헤더와 본문으로 HTML 구조를 생성
    notification.innerHTML = `
        <div class="toast-content">
            ${headerText ? `<div class="toast-header">${headerText}</div>` : ''}
            <div class="toast-body">${bodyText}</div>
        </div>
        <button class="close-btn">&times;</button>
    `;

    container.appendChild(notification);

    setTimeout(() => {
        notification.classList.add('show');
    }, 10);

    const closeNotification = () => {
        // 이미 닫히는 중이라면 중복 실행 방지
        if (!notification.classList.contains('show')) return;

        notification.classList.remove('show');
        notification.addEventListener('transitionend', () => {
            if (notification.parentElement) {
                notification.remove();
            }
            updateCloseAllButtonVisibility();
        });
    };

    notification.querySelector('.close-btn').onclick = () => closeNotification();

    container.scrollTop = container.scrollHeight;
    updateCloseAllButtonVisibility();

    setTimeout(() => {
        closeNotification();
    }, 30000);
}

// 이미지 실시간 업데이트 로직
setInterval(() => {
    const timestamp = new Date().getTime();
    // .gallery-item 내부의 순수 'img' 태그만 선택
    const allImages = document.querySelectorAll('.gallery-item:not(.empty) > img');

    allImages.forEach(img => {
        let originalSrc = img.src.split('?')[0];
        img.src = `${originalSrc}?t=${timestamp}`;
    });
}, 200); // 200ms 주기는 이미지에만 적용됨

// 로그 업데이트 로직
setInterval(() => {
    fetch('./app.log', { cache: 'no-store' })
        .then(response => {
            if (!response.ok) {
                throw new Error('로그 파일을 찾을 수 없습니다.');
            }
            return response.text();
        })
        .then(text => {
            if (!text) return;
            // textBox.innerHTML = '';
            let lines = text.split('\n');
            // if (lines.length > 50) {
            //     lines = lines.slice(lines.length - 50);
            // }

            // lines.forEach(line => {
            //     if (line.trim() !== '') {
            //         const lineElement = document.createElement('div');
            //         lineElement.className = 'text-line';
            //         lineElement.textContent = line;
            //         textBox.appendChild(lineElement);
            //     }
            // });

            // textBox.scrollTop = textBox.scrollHeight;

            const allAlarmsInLog = lines.filter(line => line.includes("ALARM"));
            if (allAlarmsInLog.length > 0) {
                const lastAlarmIndex = last_alarm ? allAlarmsInLog.lastIndexOf(last_alarm) : -1;
                const newAlarms = allAlarmsInLog.slice(lastAlarmIndex + 1);

                if (newAlarms.length > 0) {
                    // 소리 재생 (사용자 인터랙션이 없으면 브라우저가 막을 수 있음 예외처리)
                    // playAlarmPattern();

                    // 마지막 알람 상태 업데이트
                    last_alarm = newAlarms[newAlarms.length - 1];
                }

                newAlarms.forEach(alarm => {
                    createNotification(alarm);
                });

                if (newAlarms.length > 0) {
                    last_alarm = newAlarms[newAlarms.length - 1];
                }
            }
        })
        .catch(error => {
            console.error('로그 파일 로딩 오류:', error);
        });
}, 3000);

document.addEventListener('keydown', (e) => {
    // Shift 키를 누른 상태에서 'S' (대소문자 무관)를 눌렀을 때
    if (e.shiftKey && (e.key === 'S' || e.key === 's')) {
        // 이미 열려있으면 닫고, 닫혀있으면 엽니다.
        if (settingsModal.style.display === 'block') {
            settingsModal.style.display = 'none';
            backdrop.style.display = 'none';
        } else {
            settingsModal.style.display = 'block';
            backdrop.style.display = 'block';
        }
    }
});