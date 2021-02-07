#include <cmath>

#include "image.h"
#include "pinhole-camera.h"
#include "rng.h"
#include "scene.h"

// 光源の方向
const Vec3f LIGHT_DIRECTION = normalize(Vec3f(0.5, 1, 0.5));

// 反射ベクトルの計算
Vec3f reflect(const Vec3f& v, const Vec3f& n) {
  return -v + 2.0f * dot(v, n) * n;
}

// 屈折ベクトルの計算
Vec3f refract(const Vec3f& v, const Vec3f& n, float ior1, float ior2) {
  return Vec3f(0);
}

// 古典的レイトレーシングの処理
Vec3f raytrace(const Ray& ray_in, const Scene& scene) {
  constexpr int MAX_DEPTH = 100;  // 再帰の最大深さ

  Ray ray = ray_in;
  Vec3f color(0);  // 最終的な色

  IntersectInfo info;
  for (int i = 0; i < MAX_DEPTH; ++i) {
    if (scene.intersect(ray, info)) {
      if (info.hitSphere->material_type == MaterialType::Mirror) {
        // 次のレイをセット
        ray = Ray(info.hitPos, reflect(-ray.direction, info.hitNormal));
      } else if (info.hitSphere->material_type == MaterialType::Glass) {
        // 球の内部にあるか判定
        bool is_inside = dot(-ray.direction, info.hitNormal) < 0;

        // 次のレイの方向の計算
        Vec3f next_direction;
        if (!is_inside) {
          next_direction = refract(-ray.direction, info.hitNormal, 1.0f, 1.5f);
        } else {
          next_direction = refract(-ray.direction, -info.hitNormal, 1.5f, 1.0f);
        }

        // 次のレイをセット
        ray = Ray(info.hitPos, next_direction);
      } else {
        // 光源が見えるかテスト
        Ray shadow_ray(info.hitPos, LIGHT_DIRECTION);
        IntersectInfo shadow_info;
        if (!scene.intersect(shadow_ray, shadow_info)) {
          // 光源が見えたら寄与を計算
          color = std::max(dot(LIGHT_DIRECTION, info.hitNormal), 0.0f) *
                  info.hitSphere->kd;
        } else {
          // 光源から見えなかったら一定量の寄与を与える
          color = Vec3f(0.1f) * info.hitSphere->kd;
        }
      }
    } else {
      break;
    }
  }

  return color;
}

int main() {
  constexpr int width = 512;
  constexpr int height = 512;
  constexpr int SSAA_samples = 16;
  Image img(width, height);

  PinholeCamera camera(Vec3f(0, 0, 5), Vec3f(0, 0, -1));

  // シーンの作成
  Scene scene;
  scene.addSphere(
      Sphere(Vec3f(0, -1001, 0), 1000.0, Vec3f(0.9), MaterialType::Diffuse));
  scene.addSphere(Sphere(Vec3f(-1, 0, 1), 1.0, Vec3f(0.8, 0.2, 0.2),
                         MaterialType::Diffuse));
  scene.addSphere(
      Sphere(Vec3f(0), 1.0, Vec3f(0.2, 0.8, 0.2), MaterialType::Diffuse));
  scene.addSphere(Sphere(Vec3f(1, 0, -1), 1.0, Vec3f(0.2, 0.2, 0.8),
                         MaterialType::Diffuse));
  scene.addSphere(Sphere(Vec3f(-2, 2, 1), 1.0, Vec3f(1), MaterialType::Mirror));

  RNG rng;

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      Vec3f color(0);
      for (int k = 0; k < SSAA_samples; ++k) {
        // (u, v)の計算
        float u = (2.0f * (i + rng.getNext()) - width) / height;
        float v = (2.0f * (j + rng.getNext()) - height) / height;

        // レイの生成
        const Ray ray = camera.sampleRay(u, v);

        // 古典的レイトレーシングで色を計算
        color += raytrace(ray, scene);
      }

      // 平均
      color /= Vec3f(SSAA_samples);

      // 画素への書き込み
      img.setPixel(i, j, color);
    }
  }

  // PPM画像の生成
  img.writePPM("output.ppm");

  return 0;
}