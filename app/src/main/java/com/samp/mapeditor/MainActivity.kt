package com.samp.mapeditor

import android.app.Activity
import android.app.AlertDialog
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.Editable
import android.text.TextWatcher
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.*
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream

class MainActivity : Activity() {

    private lateinit var mapView: MapView
    private lateinit var inspectorPanel: LinearLayout
    private lateinit var txtSelectedInfo: TextView
    private lateinit var txtCoordInfo: TextView
    private lateinit var txtStatus: TextView

    private val REQUEST_GTA3_IMG = 1001
    private val mainHandler = Handler(Looper.getMainLooper())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Siapkan aset internal (extract SAMP assets dari APK assets jika belum ada di internal storage)
        val sampImgFile = copyAssetToFile("samp/SAMP.img")
        val sampIdeFile = copyAssetToFile("samp/SAMP.ide")

        val modelVert = readAssetString("shaders/model.vert")
        val modelFrag = readAssetString("shaders/model.frag")
        val gridVert = readAssetString("shaders/grid.vert")
        val gridFrag = readAssetString("shaders/grid.frag")

        // Inisialisasi Native Engine
        val initOk = NativeEngine.nativeInit(
            sampImgFile.absolutePath,
            sampIdeFile.absolutePath,
            modelVert, modelFrag, gridVert, gridFrag
        )

        // Bangun Layout UI
        val rootLayout = FrameLayout(this).apply {
            layoutParams = ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
            setBackgroundColor(Color.BLACK)
        }

        // 1. 3D OpenGL MapView
        mapView = MapView(this).apply {
            layoutParams = FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT)
            onObjectSelectedListener = { jsonInfo ->
                mainHandler.post { updateInspector(jsonInfo) }
            }
        }
        rootLayout.addView(mapView)

        // 2. Top Bar (Status & Action Buttons)
        val topBar = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(24, 24, 24, 24)
            setBackgroundColor(Color.parseColor("#CC121820"))
            layoutParams = FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT).apply {
                gravity = Gravity.TOP
            }
        }

        txtStatus = TextView(this).apply {
            text = if (initOk) "SA-MP 0.3.7 Engine: READY" else "Engine: INITIALIZING"
            setTextColor(Color.parseColor("#4CAF50"))
            textSize = 12f
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1.0f)
        }
        topBar.addView(txtStatus)

        val btnImportGta3 = Button(this).apply {
            text = "Mount gta3.img"
            textSize = 11f
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#1E88E5"))
            setOnClickListener { openGTA3Picker() }
        }
        topBar.addView(btnImportGta3)

        val btnExport = Button(this).apply {
            text = "Export .PWN"
            textSize = 11f
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#2E7D32"))
            setOnClickListener { showExportDialog() }
        }
        topBar.addView(btnExport)

        rootLayout.addView(topBar)

        // 3. Floating Action Buttons (Add Object & Camera Fly Controls)
        val fabAdd = Button(this).apply {
            text = "+ Object"
            textSize = 14f
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#FF6F00"))
            layoutParams = FrameLayout.LayoutParams(220, 120).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = 240
                rightMargin = 40
            }
            setOnClickListener { showObjectCatalogDialog() }
        }
        rootLayout.addView(fabAdd)

        // 4. On-screen Flycam Controls (WASD / Altitude)
        val flycamControls = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            layoutParams = FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
                gravity = Gravity.BOTTOM or Gravity.START
                leftMargin = 40
                bottomMargin = 40
            }
        }

        fun makeCamBtn(label: String, f: Float, r: Float, u: Float) = Button(this).apply {
            text = label
            textSize = 12f
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#88202530"))
            layoutParams = LinearLayout.LayoutParams(130, 110).apply { setMargins(4, 4, 4, 4) }
            setOnClickListener {
                NativeEngine.nativeCameraMove(f, r, u, 0.4f)
            }
        }

        val row1 = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        row1.addView(makeCamBtn("▲ Up", 0f, 0f, 1f))
        row1.addView(makeCamBtn("Fwd", 1f, 0f, 0f))
        row1.addView(makeCamBtn("▼ Dn", 0f, 0f, -1f))
        flycamControls.addView(row1)

        val row2 = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        row2.addView(makeCamBtn("◀ Left", 0f, -1f, 0f))
        row2.addView(makeCamBtn("Back", -1f, 0f, 0f))
        row2.addView(makeCamBtn("Right ▶", 0f, 1f, 0f))
        flycamControls.addView(row2)

        rootLayout.addView(flycamControls)

        // 5. Bottom Inspector Panel
        inspectorPanel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(30, 20, 30, 20)
            setBackgroundColor(Color.parseColor("#DD182230"))
            visibility = View.GONE
            layoutParams = FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
                gravity = Gravity.BOTTOM
            }
        }

        txtSelectedInfo = TextView(this).apply {
            text = "Selected Object"
            textSize = 14f
            setTextColor(Color.YELLOW)
        }
        inspectorPanel.addView(txtSelectedInfo)

        txtCoordInfo = TextView(this).apply {
            text = "X: 0.00 Y: 0.00 Z: 0.00 | Rot: 0, 0, 0"
            textSize = 12f
            setTextColor(Color.WHITE)
        }
        inspectorPanel.addView(txtCoordInfo)

        val actionRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            setPadding(0, 10, 0, 0)
        }

        val btnDuplicate = Button(this).apply {
            text = "Duplicate"
            textSize = 11f
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#0277BD"))
            setOnClickListener {
                NativeEngine.nativeDuplicateSelected()
                Toast.makeText(this@MainActivity, "Object Duplicated!", Toast.LENGTH_SHORT).show()
            }
        }
        actionRow.addView(btnDuplicate)

        val btnDelete = Button(this).apply {
            text = "Delete"
            textSize = 11f
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#C62828"))
            setOnClickListener {
                NativeEngine.nativeDeleteSelected()
                inspectorPanel.visibility = View.GONE
            }
        }
        actionRow.addView(btnDelete)

        val btnDeselect = Button(this).apply {
            text = "Deselect"
            textSize = 11f
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#546E7A"))
            setOnClickListener {
                inspectorPanel.visibility = View.GONE
            }
        }
        actionRow.addView(btnDeselect)

        inspectorPanel.addView(actionRow)
        rootLayout.addView(inspectorPanel)

        setContentView(rootLayout)
    }

    private fun updateInspector(jsonInfo: String) {
        if (jsonInfo.isEmpty()) {
            inspectorPanel.visibility = View.GONE
            return
        }

        try {
            val obj = JSONObject(jsonInfo)
            val name = obj.getString("name")
            val modelId = obj.getInt("modelId")
            val x = obj.getDouble("x")
            val y = obj.getDouble("y")
            val z = obj.getDouble("z")
            val rx = obj.getDouble("rx")
            val ry = obj.getDouble("ry")
            val rz = obj.getDouble("rz")

            txtSelectedInfo.text = "Object: $name (ID: $modelId)"
            txtCoordInfo.text = String.format("Pos: (%.2f, %.2f, %.2f) | Rot: (%.1f, %.1f, %.1f)", x, y, z, rx, ry, rz)
            inspectorPanel.visibility = View.VISIBLE
        } catch (e: Exception) {
            inspectorPanel.visibility = View.GONE
        }
    }

    private fun showObjectCatalogDialog() {
        val dialogView = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(30, 20, 30, 20)
        }

        val inputSearch = EditText(this).apply {
            hint = "Cari ID objek atau nama (misal: 19800 atau gate)..."
        }
        dialogView.addView(inputSearch)

        val listView = ListView(this).apply {
            layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 800)
        }
        dialogView.addView(listView)

        var currentResults = JSONArray(NativeEngine.nativeSearchObjects("", 50))
        fun refreshList(query: String) {
            currentResults = JSONArray(NativeEngine.nativeSearchObjects(query, 50))
            val items = ArrayList<String>()
            for (i in 0 until currentResults.length()) {
                val item = currentResults.getJSONObject(i)
                items.add("${item.getInt("id")} - ${item.getString("name")}")
            }
            listView.adapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, items)
        }

        refreshList("")

        inputSearch.addTextChangedListener(object : TextWatcher {
            override fun afterTextChanged(s: Editable?) { refreshList(s.toString()) }
            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
        })

        val dialog = AlertDialog.Builder(this)
            .setTitle("Katalog Objek SA-MP")
            .setView(dialogView)
            .setNegativeButton("Tutup", null)
            .create()

        listView.setOnItemClickListener { _, _, position, _ ->
            val selected = currentResults.getJSONObject(position)
            val modelId = selected.getInt("id")
            NativeEngine.nativeSpawnObjectInFrontOfCamera(modelId, 8.0f)
            Toast.makeText(this, "Spawned: ${selected.getString("name")} (ID: $modelId)", Toast.LENGTH_SHORT).show()
            dialog.dismiss()
        }

        dialog.show()
    }

    private fun showExportDialog() {
        val pawnCode = NativeEngine.nativeExportPawn(0) // 0 = Streamer CreateDynamicObject

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(30, 20, 30, 20)
        }

        val txtCode = EditText(this).apply {
            setText(pawnCode)
            isFocusable = true
            layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 700)
        }
        layout.addView(txtCode)

        AlertDialog.Builder(this)
            .setTitle("Export Kode Pawn SA-MP")
            .setView(layout)
            .setPositiveButton("Salin ke Clipboard") { _, _ ->
                val clipboard = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
                val clip = ClipData.newPlainText("SAMP Map Code", pawnCode)
                clipboard.setPrimaryClip(clip)
                Toast.makeText(this, "Kode berhasil disalin!", Toast.LENGTH_SHORT).show()
            }
            .setNegativeButton("Tutup", null)
            .show()
    }

    private fun openGTA3Picker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
        }
        startActivityForResult(intent, REQUEST_GTA3_IMG)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_GTA3_IMG && resultCode == RESULT_OK && data != null) {
            val uri: Uri? = data.data
            if (uri != null) {
                contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
                val pfd = contentResolver.openFileDescriptor(uri, "r")
                if (pfd != null) {
                    val fd = pfd.fd
                    val length = pfd.statSize
                    val ok = NativeEngine.nativeLoadGTA3Fd(fd, length)
                    if (ok) {
                        txtStatus.text = "GTA3.img: MOUNTED (${length / (1024 * 1024)} MB)"
                        txtStatus.setTextColor(Color.parseColor("#4CAF50"))
                        Toast.makeText(this, "gta3.img berhasil dimuat!", Toast.LENGTH_SHORT).show()
                    } else {
                        Toast.makeText(this, "Gagal mem-parse format gta3.img", Toast.LENGTH_LONG).show()
                    }
                }
            }
        }
    }

    private fun copyAssetToFile(assetName: String): File {
        val outFile = File(filesDir, assetName)
        outFile.parentFile?.mkdirs()
        if (!outFile.exists() || outFile.length() == 0L) {
            assets.open(assetName).use { input ->
                FileOutputStream(outFile).use { output ->
                    input.copyTo(output)
                }
            }
        }
        return outFile
    }

    private fun readAssetString(assetName: String): String {
        return assets.open(assetName).bufferedReader().use { it.readText() }
    }
}
